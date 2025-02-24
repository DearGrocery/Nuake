#include "Mesh.h"

#include "src/Core/Maths.h"
#include "src/Rendering/Textures/Material.h"
#include "src/Rendering/Textures/MaterialManager.h"
#include "src/Rendering/Renderer.h"
#include "src/Rendering/Shaders/Shader.h"
#include "src/Rendering/Vertex.h"
#include "src/Rendering/Buffers/VertexBuffer.h"
#include "src/Rendering/Buffers/VertexArray.h"
#include "src/Rendering/Buffers/VertexBufferLayout.h"

namespace Nuake
{
    Mesh::Mesh() {}
    Mesh::~Mesh() {}

    void Mesh::AddSurface(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
    {
        this->m_Vertices = vertices;
        this->m_Indices = indices;

        SetupMesh();
        CalculateAABB();

        if (m_Material == nullptr)
        {
            m_Material = MaterialManager::Get()->GetMaterial("default");
        }
    }

    std::vector<Vertex>& Mesh::GetVertices()
    {
        return m_Vertices;
    }

    std::vector<uint32_t>& Mesh::GetIndices()
    {
        return m_Indices;
    }

    Ref<Material> Mesh::GetMaterial() inline const
    {
        return m_Material;
    }

    void Mesh::SetMaterial(Ref<Material> material)
    {
        m_Material = material;
        MaterialManager::Get()->RegisterMaterial(material);
    }
	
	void Mesh::CalculateAABB()
	{
        float minX = 0.0f;
        float minY = 0.0f;
        float minZ = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        float maxZ = 0.0f;

		for (const Vertex& v : m_Vertices)
		{
			minX = v.position.x < minX ? v.position.x : minX;
			minY = v.position.y < minY ? v.position.y : minY;
			minZ = v.position.z < minZ ? v.position.z : minZ;
			maxX = v.position.x > maxX ? v.position.x : maxX;
			maxY = v.position.y > maxY ? v.position.y : maxY;
			maxZ = v.position.z > maxZ ? v.position.z : maxZ;
		}

        this->m_AABB = { Vector3(minX, minY, minZ), Vector3(maxX, maxY, maxZ) };
	}

    void Mesh::SetupMesh()
    {
        m_VertexArray = CreateScope<VertexArray>();
        m_VertexArray->Bind();
        m_VertexBuffer = CreateScope<VertexBuffer>(m_Vertices.data(), m_Vertices.size() * sizeof(Vertex));
        m_ElementBuffer = CreateScope<VertexBuffer>(m_Indices.data(), m_Indices.size() * sizeof(unsigned int), RendererEnum::ELEMENT_ARRAY_BUFFER);

        VertexBufferLayout bufferLayout = VertexBufferLayout();
        bufferLayout.Push<float>(3); // Position
        bufferLayout.Push<float>(2); // UV
        bufferLayout.Push<float>(3); // Normal
        bufferLayout.Push<float>(3); // Tangent
        bufferLayout.Push<float>(3); // Bitangent
        bufferLayout.Push<float>(1); // Texture

        m_VertexArray->AddBuffer(*m_VertexBuffer, bufferLayout);
        m_VertexArray->Unbind();
    }

    void Mesh::Draw(Shader* shader, bool bindMaterial)
    {
        if (bindMaterial)
            m_Material->Bind(shader);

        m_VertexArray->Bind();
        RenderCommand::DrawElements(RendererEnum::TRIANGLES, (int)m_Indices.size(), RendererEnum::UINT, 0);
    }

    void Mesh::DebugDraw()
    {
        Renderer::m_DebugShader->Bind();
        Renderer::m_DebugShader->SetUniform4f("u_Color", 1.0f, 0.0f, 0.0f, 1.f);

        m_VertexArray->Bind();
        RenderCommand::DrawElements(RendererEnum::TRIANGLES, (int)m_Indices.size(), RendererEnum::UINT, 0);
    }

    json Mesh::Serialize()
    {
        BEGIN_SERIALIZE();

        j["Material"] = m_Material->Serialize();
        j["Indices"] = m_Indices;

		json v;
        for (uint32_t i = 0; i < m_Vertices.size(); i++)
        {
			v["Position"]["x"] = m_Vertices[i].position.x;
			v["Position"]["y"] = m_Vertices[i].position.y;
			v["Position"]["z"] = m_Vertices[i].position.z;

			v["UV"]["x"] = m_Vertices[i].uv.x;
			v["UV"]["y"] = m_Vertices[i].uv.y;

			v["Normal"]["x"] = m_Vertices[i].normal.x;
			v["Normal"]["y"] = m_Vertices[i].normal.y;
			v["Normal"]["z"] = m_Vertices[i].normal.z;

			v["Tangent"]["x"] = m_Vertices[i].tangent.x;
			v["Tangent"]["y"] = m_Vertices[i].tangent.y;
			v["Tangent"]["z"] = m_Vertices[i].tangent.z;

			v["Bitangent"]["x"] = m_Vertices[i].bitangent.x;
			v["Bitangent"]["y"] = m_Vertices[i].bitangent.y;
			v["Bitangent"]["z"] = m_Vertices[i].bitangent.z;

            j["Vertices"][i] = v;
        }


        END_SERIALIZE();
    }

    bool Mesh::Deserialize(const std::string& str)
    {
        BEGIN_DESERIALIZE();
         
        m_Material = CreateRef<Material>();
        m_Material->Deserialize(j["Material"].dump());

        std::vector<uint32_t> indices;
        for (auto& i : j["Indices"])
        {
            indices.push_back(i);
        }
        m_Indices = indices;

        std::vector<Vertex> vertices;
        for (auto& v : j["Vertices"])
        {
            Vertex vertex;
            DESERIALIZE_VEC3(v["Position"], vertex.position)
			DESERIALIZE_VEC3(v["Normal"], vertex.normal)
			DESERIALIZE_VEC2(v["UV"], vertex.uv)
			DESERIALIZE_VEC3(v["Tangent"], vertex.tangent)
			DESERIALIZE_VEC3(v["Bitangent"], vertex.bitangent)

            vertices.push_back(vertex);
        }

        m_Vertices = vertices;

		SetupMesh();
        return true;
    }
}
