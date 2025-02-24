#include "PhysicsSystem.h"
#include "src/Scene/Scene.h"
#include "src/Scene/Components/BoxCollider.h"
#include "src/Scene/Components/CapsuleColliderComponent.h"
#include "src/Scene/Components/SphereCollider.h"
#include "src/Scene/Components/MeshCollider.h"
#include "src/Scene/Components/ModelComponent.h"
#include <src/Scene/Components/RigidbodyComponent.h>
#include "src/Scene/Entities/Entity.h"
#include <src/Core/Physics/PhysicsManager.h>
#include <src/Scene/Components/CharacterControllerComponent.h>
#include <src/Scene/Components/QuakeMap.h>
#include <src/Scene/Components/BSPBrushComponent.h>
#include <src/Scene/Components/TriggerZone.h>

namespace Nuake
{
	PhysicsSystem::PhysicsSystem(Scene* scene)
	{
		m_Scene = scene;
	}

	bool PhysicsSystem::Init()
	{
		// Create physic world.
		auto view = m_Scene->m_Registry.view<TransformComponent, RigidBodyComponent>();
		for (auto e : view)
		{
			auto [transform, rigidBodyComponent] = view.get<TransformComponent, RigidBodyComponent>(e);
			Entity ent = Entity({ e, m_Scene });
			Ref<Physics::RigidBody> rigidBody;
			if (ent.HasComponent<BoxColliderComponent>())
			{
				float mass = rigidBodyComponent.Mass;

				BoxColliderComponent& boxComponent = ent.GetComponent<BoxColliderComponent>();
				Ref<Physics::Box> boxShape = CreateRef<Physics::Box>(boxComponent.Size);

				rigidBody = CreateRef<Physics::RigidBody>(rigidBodyComponent.Mass, transform.GetGlobalPosition(), transform.GetGlobalTransform(), boxShape, ent);
				PhysicsManager::Get().RegisterBody(rigidBody);
 			}

			if (ent.HasComponent<CapsuleColliderComponent>())
			{
				auto& capsuleComponent = ent.GetComponent<CapsuleColliderComponent>();
				float radius = capsuleComponent.Radius;
				float height = capsuleComponent.Height;
				auto capsuleShape = CreateRef<Physics::Capsule>(radius, height);

				rigidBody = CreateRef<Physics::RigidBody>(rigidBodyComponent.Mass, transform.GetGlobalPosition(), transform.GetGlobalTransform(), capsuleShape, ent);
				PhysicsManager::Get().RegisterBody(rigidBody);
			}

			if (ent.HasComponent<SphereColliderComponent>())
			{
				float mass = rigidBodyComponent.Mass;

				const auto& component = ent.GetComponent<SphereColliderComponent>();
				auto shape = CreateRef<Physics::Sphere>(component.Radius);

				rigidBody = CreateRef<Physics::RigidBody>(rigidBodyComponent.Mass, transform.GetGlobalPosition(), transform.GetGlobalTransform(), shape, ent);
				PhysicsManager::Get().RegisterBody(rigidBody);
			}

			if (ent.HasComponent<MeshColliderComponent>())
			{
				if (!ent.HasComponent<ModelComponent>())
				{
					Logger::Log("Cannot use mesh collider without model component", WARNING);
				}
				const auto& modelComponent = ent.GetComponent<ModelComponent>();
				const auto& component = ent.GetComponent<MeshColliderComponent>();

				if (modelComponent.ModelResource)
				{
					uint32_t subMeshId = component.SubMesh;
					const std::vector<Ref<Mesh>>& submeshes = modelComponent.ModelResource->GetMeshes();
					if (subMeshId >= submeshes.size())
					{
						Logger::Log("Cannot create mesh collider, invalid submesh ID", WARNING);
					}
					Ref<Mesh> mesh = submeshes[subMeshId];
					auto shape = CreateRef<Physics::MeshShape>(mesh);
					rigidBody = CreateRef<Physics::RigidBody>(rigidBodyComponent.Mass, transform.GetGlobalPosition(), transform.GetGlobalTransform(), shape, ent);
					PhysicsManager::Get().RegisterBody(rigidBody);
				}
			}
		}

		const auto characterControllerView = m_Scene->m_Registry.view<TransformComponent, CharacterControllerComponent>();
		for (auto e : characterControllerView)
		{
			auto [transformComponent, characterControllerComponent] = view.get<TransformComponent, RigidBodyComponent>(e);

		}

		//// character controllers
		//auto ccview = m_Scene->m_Registry.view<TransformComponent, CharacterControllerComponent>();
		//for (auto e : ccview)
		//{
		//	auto [transform, cc] = ccview.get<TransformComponent, CharacterControllerComponent>(e);

		//	cc.CharacterController = CreateRef<Physics::CharacterController>(cc.Height, cc.Radius, cc.Mass, transform.GetLocalPosition());
		//	Entity ent = Entity({ e, m_Scene });
		//	cc.CharacterController->SetEntity(ent);

		//	PhysicsManager::Get()->RegisterCharacterController(cc.CharacterController);
		//}

		auto bspView = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent, ModelComponent>();
		for (auto e : bspView)
		{
			Entity ent = Entity({ e, m_Scene });
			auto [transform, brush, model] = bspView.get<TransformComponent, BSPBrushComponent, ModelComponent>(e);

			if (!brush.IsSolid)
				continue;

			for (const auto& h : brush.Hulls)
			{
				Ref<Physics::ConvexHullShape> meshShape = CreateRef<Physics::ConvexHullShape>(h);
				Ref<Physics::RigidBody> btRigidbody = CreateRef<Physics::RigidBody>(0.0f, transform.GetGlobalPosition(), transform.GetGlobalTransform(), meshShape, ent);

				btRigidbody->SetEntityID(Entity{ e, m_Scene });
				brush.Rigidbody.push_back(btRigidbody);
				PhysicsManager::Get().RegisterBody(btRigidbody);
			}
		}

		//auto bspTriggerView = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent, TriggerZone>();
		//for (auto e : bspTriggerView) 
		//{
		//	auto [transform, brush, trigger] = bspTriggerView.get<TransformComponent, BSPBrushComponent, TriggerZone>(e);

		//	Ref<Physics::MeshShape> meshShape = CreateRef<Physics::MeshShape>(brush.Meshes[0]);
		//	Ref<GhostObject> ghostBody = CreateRef<GhostObject>(transform.GetGlobalPosition(), meshShape);
		//	trigger.GhostObject = ghostBody;

		//	PhysicsManager::Get()->RegisterGhostBody(ghostBody);
		//}
		return true;
	}

	void PhysicsSystem::Update(Timestep ts)
	{
		if (!Engine::IsPlayMode)
			return;

		auto brushes = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent>();
		for (auto e : brushes)
		{
			auto [transform, brush] = brushes.get<TransformComponent, BSPBrushComponent>(e);

			for (auto& r : brush.Rigidbody)
			{
				//r->m_Transform->setOrigin(btVector3(transform.GlobalTranslation.x, transform.GlobalTranslation.y, transform.GlobalTranslation.z));
				//r->UpdateTransform(*r->m_Transform);
			}

			if (!brush.IsFunc)
				continue;

			brush.Targets.clear();
			auto targetnameView = m_Scene->m_Registry.view<TransformComponent, NameComponent>();
			for (auto e2 : targetnameView)
			{
				auto [ttransform, name] = targetnameView.get<TransformComponent, NameComponent>(e2);

				if (name.Name == brush.target)
				{
					brush.Targets.push_back(Entity{ e2, m_Scene });
				}
			}
		}

		auto bspTriggerView = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent, TriggerZone>();
		for (auto e : bspTriggerView)
		{
			auto [transform, brush, trigger] = bspTriggerView.get<TransformComponent, BSPBrushComponent, TriggerZone>(e);
			trigger.GhostObject->ScanOverlap();

			brush.Targets.clear();
			auto targetnameView = m_Scene->m_Registry.view<TransformComponent, NameComponent>();
			for (auto e2 : targetnameView)
			{
				auto [ttransform, name] = targetnameView.get<TransformComponent, NameComponent>(e2);

				if (name.Name == brush.target) {
					brush.Targets.push_back(Entity{ e2, m_Scene });
				}
			}
		}

		auto physicGroup = m_Scene->m_Registry.view<TransformComponent, RigidBodyComponent>();
		for (auto e : physicGroup) {
			auto [transform, rb] = physicGroup.get<TransformComponent, RigidBodyComponent>(e);
			rb.SyncTransformComponent(&m_Scene->m_Registry.get<TransformComponent>(e));
		}

		auto ccGroup = m_Scene->m_Registry.view<TransformComponent, CharacterControllerComponent>();
		for (auto e : ccGroup) {
			auto [transform, rb] = ccGroup.get<TransformComponent, CharacterControllerComponent>(e);
			rb.SyncWithTransform(m_Scene->m_Registry.get<TransformComponent>(e));
		}
	}

	void PhysicsSystem::FixedUpdate(Timestep ts)
	{
		if (!Engine::IsPlayMode)
			return;

		PhysicsManager::Get().Step(ts);
	}

	void PhysicsSystem::Exit()
	{
		PhysicsManager::Get().Reset();
	}
}
