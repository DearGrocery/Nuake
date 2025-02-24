#shader vertex
#version 460 core
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 UVPosition;

out flat vec2 UV;
out mat4 InvProjection;
out mat4 InvView;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    UV = UVPosition;
    InvProjection = inverse(u_Projection);
    InvView = inverse(u_View);

    gl_Position = vec4(VertexPosition, 1.0f);
}

#shader fragment
#version 460 core

out vec4 FragColor;

in vec2 UV;
in mat4 InvProjection;
in mat4 InvView;

// Camera
uniform float u_Exposure;
uniform vec3  u_EyePosition;

// GBuffer
uniform sampler2D m_Depth;
uniform sampler2D m_Albedo; 
uniform sampler2D m_Material; 
uniform sampler2D m_Normal;
uniform sampler2D m_SSAO;

// Lights
const int MaxLight = 29;
uniform int LightCount = 0;

struct Light {
    int Type; // 0 = directional, 1 = point
    vec3 Direction;
    vec3 Color;
    float Strength;
    vec3 Position;
    int ShadowMapsIDs[4];
    float CascadeDepth[4];
    mat4 LightTransforms[4];
    int Volumetric;
};

uniform sampler2D ShadowMaps[4];

uniform Light Lights[MaxLight];

// Converts depth to World space coords.
vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(UV * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = InvProjection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = InvView * viewSpacePosition;

    return worldSpacePosition.xyz;
}

const float PI = 3.141592653589793f;
float height_scale = 0.02f;

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

int GetCSMDepth(float depth, Light light)
{
    int shadowmap = -1;
    // Get CSM depth
    for (int i = 0; i < 4; i++)
    {
        float CSMDepth = light.CascadeDepth[i];

        if (depth < CSMDepth + 0.0001f)
        {
            shadowmap = i;
            break;
        }
    }

    return shadowmap;
}

float SampleShadowMap(sampler2D shadowMap, vec2 coords, float compare)
{
    return step(compare, texture(shadowMap, coords.xy).r);
}

float SampleShadowMapLinear(sampler2D shadowMap, vec2 coords, float compare, vec2 texelSize)
{
    vec2 pixelPos = coords / texelSize + vec2(0.5);
    vec2 fracPart = fract(pixelPos);
    vec2 startTexel = (pixelPos - fracPart) * texelSize;

    float blTexel = SampleShadowMap(shadowMap, startTexel, compare);
    float brTexel = SampleShadowMap(shadowMap, startTexel + vec2(texelSize.x, 0.0), compare);
    float tlTexel = SampleShadowMap(shadowMap, startTexel + vec2(0.0, texelSize.y), compare);
    float trTexel = SampleShadowMap(shadowMap, startTexel + texelSize, compare);

    float mixA = mix(blTexel, tlTexel, fracPart.y);
    float mixB = mix(brTexel, trTexel, fracPart.y);

    return mix(mixA, mixB, fracPart.x);
}

float ShadowCalculation(Light light, vec3 FragPos, vec3 normal)
{
    // Get Depth
    float depth = length(FragPos - u_EyePosition);
    int shadowmap = GetCSMDepth(depth, light);
    if (shadowmap == -1)
        return 1.0;

    vec4 fragPosLightSpace = light.LightTransforms[shadowmap] * vec4(FragPos, 1.0f);

    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = 0.0;
    float bias = max(0.005 * (1.0 - dot(normal, light.Direction)), 0.0005);
    //float pcfDepth = texture(ShadowMaps[shadowmap], vec3(projCoords.xy, currentDepth), bias);

    if (shadowmap <= 3)
    {
        const float NUM_SAMPLES = 4.f;
        const float SAMPLES_START = (NUM_SAMPLES - 1.0f) / 2.0f;
        const float NUM_SAMPLES_SQUARED = NUM_SAMPLES * NUM_SAMPLES;
        vec2 texelSize = 1.0 / vec2(2048, 2048);

        float result = 0.0f;
        for (float y = -SAMPLES_START; y <= SAMPLES_START; y += 1.0f)
        {
            for (float x = -SAMPLES_START; x <= SAMPLES_START; x += 1.0f)
            {
                vec2 coordsOffset = vec2(x, y) * texelSize;
                result += SampleShadowMapLinear(ShadowMaps[shadowmap], projCoords.xy + coordsOffset, currentDepth - bias, texelSize);
            }
        }

        return result / NUM_SAMPLES_SQUARED;
    }
    
    else
    {
        return SampleShadowMap(ShadowMaps[shadowmap], projCoords.xy, currentDepth - bias);
    }
   
}

void main()
{
    vec3 worldPos = WorldPosFromDepth(texture(m_Depth, UV).r);
    if (texture(m_Depth, UV).r == 1) 
    {
        FragColor = vec4(0, 0, 0, 0);
        return;
    }

    // Convert from [0, 1] to [-1, 1].
    vec3 albedo      = texture(m_Albedo, UV).rgb;
    vec3 normal      = texture(m_Normal, UV).rgb * 2.0 - 1.0;
    float metallic   = texture(m_Material, UV).r;
    float roughness  = texture(m_Material, UV).b;
    float ao         = texture(m_Material, UV).g;
    float ssao      = texture(m_SSAO, UV).r;
    vec3 N = normal;
    vec3 V = normalize(u_EyePosition - worldPos);
    vec3 R = reflect(-V, N);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 eyeDirection = normalize(u_EyePosition - worldPos);

    vec3 Lo = vec3(0.0);
    vec3 fog = vec3(0.0);
    float shadow = 0.0f;
    for (int i = 0; i < LightCount; i++)
    {
        vec3 L = normalize(Lights[i].Position - worldPos);

        float distance = length(Lights[i].Position - worldPos);
        float attenuation = 1.0 / (distance * distance);

        if (Lights[i].Type == 0) {
            L = normalize(Lights[i].Direction);
            attenuation = 1.0f;
            shadow += ShadowCalculation(Lights[i], worldPos, N);
        }

        vec3 radiance = Lights[i].Color * attenuation ;

        if (Lights[i].Type == 0) {
            radiance *= shadow;
        }

        // Cook-Torrance BRDF
        vec3 H = normalize(V + L);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 nominator = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI) * radiance * NdotL;// note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    /// ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 ambient = (kD * albedo) * (ao);
    vec3 color = (ambient * ssao) + Lo;

    // Display CSM splits..
    /*float depth = length(worldPos - u_EyePosition);
    int cascade = GetCSMDepth(depth, Lights[0]);
    if (cascade == 0)
        color *= vec3(1.0, 0.0, 0.0);
    if (cascade == 1)
        color *= vec3(0.0, 1.0, 0.0);
    if (cascade == 2)
        color *= vec3(0.0, 0.0, 1.0);
    if (cascade == 3)
        color *= vec3(0.0, 1.0, 1.0);
    */

    FragColor = vec4(color, 1.0);
}