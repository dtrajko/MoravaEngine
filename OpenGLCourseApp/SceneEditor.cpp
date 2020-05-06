#include "SceneEditor.h"

#include "ImGuiWrapper.h"
#include "MousePicker.h"
#include "GeometryFactory.h"

#include <vector>
#include <string>


SceneEditor::SceneEditor()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_Selected = 1;

    m_Position_1 = glm::vec3(2.0f, 0.5f, 2.0f);
    m_Position_2 = glm::vec3(-2.0f, 0.5f, -2.0f);

    m_Scale_1 = glm::vec3(1.0f);
    m_Scale_2 = glm::vec3(1.0f);

    m_Color_1 = glm::vec4(0.8f, 0.4f, 0.0f, 0.8f);
    m_Color_2 = glm::vec4(0.4f, 0.4f, 0.8f, 0.8f);

    m_AABB_1 = new AABB(m_Position_1, m_Scale_1);
    m_AABB_2 = new AABB(m_Position_2, m_Scale_2);

    m_Pivot_1 = new Pivot(m_Position_1, m_Scale_1 + 1.0f);
    m_Pivot_2 = new Pivot(m_Position_2, m_Scale_2 + 1.0f);

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };
}

void SceneEditor::SetSkybox()
{
}

void SceneEditor::SetTextures()
{
    textures.insert(std::make_pair("semi_transparent", new Texture("Textures/semi_transparent.png")));
}

void SceneEditor::SetupMeshes()
{
    m_Quad = new Quad();
    meshes.insert(std::make_pair("quad", m_Quad));
}

void SceneEditor::SetupModels()
{
}

void SceneEditor::SetGeometry()
{
    GeometryFactory::CubeNormals::Create();
}

void SceneEditor::Update(float timestep, Window& mainWindow)
{
    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    m_IsSelected_1 = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), m_AABB_1->GetMin(), m_AABB_1->GetMax(), glm::vec2(0.0f));
    m_IsSelected_2 = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), m_AABB_2->GetMin(), m_AABB_2->GetMax(), glm::vec2(0.0f));

    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        if (m_IsSelected_1) m_Selected = 1;
        else if (m_IsSelected_2) m_Selected = 2;
    }
}

void SceneEditor::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    MousePicker* mp = MousePicker::Get();

    ImGui::Begin("Scene Editor");

    ImGui::Separator();
    std::string mouseCoords = "Mouse Coordinates: MouseX = " + std::to_string(mp->m_MouseX) +
        " MouseY = " + std::to_string(mp->m_MouseY);
    ImGui::Text(mouseCoords.c_str());
    ImGui::Separator();
    std::string normalizedCoords = "Normalized Coords: X = " + std::to_string(mp->m_NormalizedCoords.x) +
                                                     " Y = " + std::to_string(mp->m_NormalizedCoords.y);
    ImGui::Text(normalizedCoords.c_str());
    ImGui::Separator();
    std::string clipCoords = "Clip Coords: X = " + std::to_string(mp->m_ClipCoords.x) +
                                         " Y = " + std::to_string(mp->m_ClipCoords.y);
    ImGui::Text(clipCoords.c_str());
    ImGui::Separator();
    std::string eyeCoords = "Eye Coords: X = " + std::to_string(mp->m_EyeCoords.x) + " Y = " + std::to_string(mp->m_EyeCoords.y) +
                                       " Z = " + std::to_string(mp->m_EyeCoords.z) + " W = " + std::to_string(mp->m_EyeCoords.w);
    ImGui::Text(eyeCoords.c_str());
    ImGui::Separator();
    std::string worldRay = "World Ray: X = " + std::to_string(mp->m_WorldRay.x) +
                                     " Y = " + std::to_string(mp->m_WorldRay.y) +
                                     " Z = " + std::to_string(mp->m_WorldRay.z);
    ImGui::Text(worldRay.c_str());
    ImGui::Separator();
    ImGui::SliderFloat3("Ray Start", glm::value_ptr(m_Raycast->m_LineStart), -10.0f, 10.0f);
    ImGui::SliderFloat3("Ray End",   glm::value_ptr(m_Raycast->m_LineEnd),   -10.0f, 10.0f);
    ImGui::Separator();
    ImGui::SliderFloat3("Intersection point", glm::value_ptr(mp->m_IntersectionPoint), -10.0f, 10.0f);
    ImGui::Separator();

    if (m_Selected == 1) {
        m_PositionEdit = &m_Position_1;
        m_ScaleEdit = &m_Scale_1;
        m_ColorEdit = &m_Color_1;
    }
    else if (m_Selected == 2) {
        m_PositionEdit = &m_Position_2;
        m_ScaleEdit = &m_Scale_2;
        m_ColorEdit = &m_Color_2;
    }

    ImGui::Text("Transform");
    ImGui::SliderFloat3("Position", (float*)m_PositionEdit, -10.0f, 10.0f);
    ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.0f, 10.0f);
    ImGui::ColorEdit4("Color", (float*)m_ColorEdit);
    ImGui::SliderInt("Selected Object", &m_Selected, 1, 2);

    ImGui::End();
}

void SceneEditor::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    shaders["editor_object"]->Bind();
    shaders["editor_object"]->setMat4("projection", projectionMatrix);
    shaders["editor_object"]->setMat4("view", m_Camera->CalculateViewMatrix());

    m_Transform_1 = glm::mat4(1.0f);
    m_Transform_1 = glm::translate(m_Transform_1, m_Position_1);
    m_Transform_1 = glm::scale(m_Transform_1, m_Scale_1);
    shaders["editor_object"]->setMat4("model", m_Transform_1);
    shaders["editor_object"]->setVec4("tintColor", m_Color_1);
    shaders["editor_object"]->setBool("isSelected", m_IsSelected_1);
    glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    m_Transform_2 = glm::mat4(1.0f);
    m_Transform_2 = glm::translate(m_Transform_2, m_Position_2);
    m_Transform_2 = glm::scale(m_Transform_2, m_Scale_2);
    shaders["editor_object"]->setMat4("model", m_Transform_2);
    shaders["editor_object"]->setVec4("tintColor", m_Color_2);
    shaders["editor_object"]->setBool("isSelected", m_IsSelected_2);
    glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    m_AABB_1->Update(m_Position_1, m_Scale_1);
    m_AABB_2->Update(m_Position_2, m_Scale_2);

    m_Pivot_1->Update(m_Position_1, m_Scale_1 + 1.0f);
    m_Pivot_2->Update(m_Position_2, m_Scale_2 + 1.0f);

    if (m_Selected == 1) {
        m_AABB_1->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
        m_Pivot_1->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    }
    else if (m_Selected == 2) {
        m_AABB_2->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
        m_Pivot_2->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    }    
}

void SceneEditor::CleanupGeometry()
{
    GeometryFactory::CubeNormals::Destroy();
}

SceneEditor::~SceneEditor()
{
	CleanupGeometry();

    delete m_Raycast;

    delete m_AABB_1;
    delete m_AABB_2;

    delete m_Pivot_1;
    delete m_Pivot_2;
}
