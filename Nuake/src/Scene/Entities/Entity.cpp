

#include "Components/ParentComponent.h"
#include "Entity.h"
#include "Components/NameComponent.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/QuakeMap.h"
#include "Components/LightComponent.h"
void Entity::AddChild(Entity ent)
{
	if ((int)m_EntityHandle != ent.GetHandle())
	{
		ent.GetComponent<ParentComponent>().HasParent = true;
		ent.GetComponent<ParentComponent>().Parent = *this;

		GetComponent<ParentComponent>().Children.push_back(ent);
	}
}

json Entity::Serialize()
{
	BEGIN_SERIALIZE();
	SERIALIZE_OBJECT_REF_LBL("NameComponent", GetComponent<NameComponent>());
	SERIALIZE_OBJECT_REF_LBL("ParentComponent", GetComponent<ParentComponent>());
	SERIALIZE_OBJECT_REF_LBL("TransformComponent", GetComponent<TransformComponent>());
	if(HasComponent<CameraComponent>())
		SERIALIZE_OBJECT_REF_LBL("CameraComponent", GetComponent<CameraComponent>());
	if(HasComponent<QuakeMap>())
		SERIALIZE_OBJECT_REF_LBL("QuakemapComponent", GetComponent<QuakeMap>());
	if (HasComponent<LightComponent>())
		SERIALIZE_OBJECT_REF_LBL("LightComponent", GetComponent<LightComponent>());
	END_SERIALIZE();
}

bool Entity::Deserialize(const std::string& str)
{
	return false;
}

Entity::Entity(entt::entity handle, Scene* scene)
{
	m_EntityHandle = handle;
	m_Scene = scene;
}

Entity::Entity(const Entity& ent)
{
	this->m_EntityHandle = ent.m_EntityHandle;
	this->m_Scene = ent.m_Scene;
}


Entity::Entity(){}