#pragma once
#include "ComponentPanel.h"
#include <src/Scene/Components/QuakeMap.h>
#include "src/Scene/Systems/QuakeMapBuilder.h"
#include <src/Core/FileSystem.h>

class QuakeMapPanel : ComponentPanel {

public:
    QuakeMapPanel() {}

    void Draw(Nuake::Entity entity) override
    {
        if (!entity.HasComponent<Nuake::QuakeMapComponent>())
            return;

        Nuake::QuakeMapComponent& component = entity.GetComponent<Nuake::QuakeMapComponent>();
        BeginComponentTable(QUAKEMAP, Nuake::QuakeMapComponent);
        {
            {
                ImGui::Text("Map");
                ImGui::TableNextColumn();

                std::string path = component.Path;
                ImGui::Button(component.Path.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_Map"))
                    {
                        char* file = (char*)payload->Data;
                        std::string fullPath = std::string(file, 256);
                        path = Nuake::FileSystem::AbsoluteToRelative(fullPath);

                        component.Path = path;
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::TableNextColumn();

                ComponentTableReset(component.Path, "");
            }
            ImGui::TableNextColumn();
            {
                ImGui::Text("Collision");
                ImGui::TableNextColumn();

                ImGui::Checkbox("##Collison", &component.HasCollisions);
                ImGui::TableNextColumn();
                ComponentTableReset(component.HasCollisions, true);
            }
            ImGui::TableNextColumn();
            {
                ImGui::Text("Build");
                ImGui::TableNextColumn();

                if (ImGui::Button("Build"))
                {
                    Nuake::QuakeMapBuilder builder;
                    builder.BuildQuakeMap(entity, component.HasCollisions);
                }

                ImGui::TableNextColumn();

                //ComponentTableReset(component.Class, "");
            }
        }
        EndComponentTable();
    }
};