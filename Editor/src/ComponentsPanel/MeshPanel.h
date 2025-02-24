#pragma once
#include <src/Core/Core.h>
#include "ComponentPanel.h"
#include "ModelResourceInspector.h"

#include <src/Scene/Entities/ImGuiHelper.h>
#include <src/Scene/Components/ModelComponent.h>

#include <src/Resource/ResourceLoader.h>
#include <src/Core/String.h>

class MeshPanel : ComponentPanel {
private:
    Scope<ModelResourceInspector> _modelInspector;
    bool _expanded = false;
public:
    MeshPanel() 
    {
        CreateScope<ModelResourceInspector>();
    }

    void Draw(Nuake::Entity entity) override
    {
        using namespace Nuake;
        if (!entity.HasComponent<ModelComponent>())
            return;

        ModelComponent& component = entity.GetComponent<ModelComponent>();
        BeginComponentTable(MESH, ModelComponent);
        {
            ImGui::Text("Model");
            ImGui::TableNextColumn();

            std::string label = "None";

            const bool isModelNone = component.ModelResource == nullptr;
            if (!isModelNone)
            {
                label = std::to_string(component.ModelResource->ID);
            }

            if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                if (!isModelNone)
                {
                    if (!_expanded)
                    {
                        _modelInspector = CreateScope<ModelResourceInspector>(component.ModelResource);
                    }

                    _expanded = !_expanded;
                }
            }

            if (_expanded)
            {
                _modelInspector->Draw();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_Model"))
                {
                    char* file = (char*)payload->Data;
                    std::string fullPath = std::string(file, 256);
                    fullPath = Nuake::FileSystem::AbsoluteToRelative(fullPath);
                    
                    if (Nuake::String::EndsWith(fullPath, ".model"))
                    {

                    }
                    else
                    {
                        component.ModelPath = fullPath;
                        component.LoadModel();
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::TableNextColumn();
            ComponentTableReset(component.ModelPath, "");
        }
        EndComponentTable();
    }
};