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

    m_Selected = 0;

    sceneObjects.resize(2);

    sceneObjects[0].position = glm::vec3(2.0f, 0.5f, 2.0f);
    sceneObjects[1].position = glm::vec3(-2.0f, 0.5f, -2.0f);

    sceneObjects[0].rotation = glm::vec3(0.0f);
    sceneObjects[1].rotation = glm::vec3(0.0f);

    sceneObjects[0].scale = glm::vec3(1.0f);
    sceneObjects[1].scale = glm::vec3(1.0f);

    sceneObjects[0].color = glm::vec4(0.8f, 0.4f, 0.0f, 0.8f);
    sceneObjects[1].color = glm::vec4(0.4f, 0.4f, 0.8f, 0.8f);

    sceneObjects[0].AABB = new AABB(sceneObjects[0].position, sceneObjects[0].scale);
    sceneObjects[1].AABB = new AABB(sceneObjects[1].position, sceneObjects[1].scale);

    sceneObjects[0].pivot = new Pivot(sceneObjects[0].position, sceneObjects[0].scale + 1.0f);
    sceneObjects[1].pivot = new Pivot(sceneObjects[1].position, sceneObjects[1].scale + 1.0f);

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

    for (auto& object : sceneObjects) {
        object.isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            object.AABB->GetMin(), object.AABB->GetMax(), glm::vec2(0.0f));
    }

    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        for (unsigned int i = 0; i < sceneObjects.size(); i++)
        {
            if (sceneObjects[i].isSelected) m_Selected = i;
        }
    }

    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1] && mainWindow.getKeys()[GLFW_KEY_LEFT_SHIFT])
    {
        AddSceneObject();
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

    if (m_Selected < sceneObjects.size())
    {
        m_PositionEdit = &sceneObjects[m_Selected].position;
        m_RotationEdit = &sceneObjects[m_Selected].rotation;
        m_ScaleEdit =    &sceneObjects[m_Selected].scale;
        m_ColorEdit =    &sceneObjects[m_Selected].color;
    }

    ImGui::Text("Transform");
    ImGui::SliderFloat3("Position", (float*)m_PositionEdit, -10.0f, 10.0f);
    // ImGui::SliderFloat3("Rotation", (float*)m_RotationEdit, -179.0f, 180.0f);
    ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.0f, 10.0f);
    ImGui::ColorEdit4("Color", (float*)m_ColorEdit);
    ImGui::SliderInt("Selected Object", &m_Selected, 0, (int)(sceneObjects.size() - 1));

    ImGui::End();
}

void SceneEditor::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    shaders["editor_object"]->Bind();
    shaders["editor_object"]->setMat4("projection", projectionMatrix);
    shaders["editor_object"]->setMat4("view", m_Camera->CalculateViewMatrix());

    for (auto& object : sceneObjects)
    {
        object.transform = glm::mat4(1.0f);
        object.transform = glm::translate(object.transform, object.position);
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        object.transform = glm::scale(object.transform, object.scale);
        shaders["editor_object"]->setMat4("model", object.transform);
        shaders["editor_object"]->setVec4("tintColor", object.color);
        shaders["editor_object"]->setBool("isSelected", object.isSelected);
        glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        object.AABB->Update(object.position, object.scale);
        object.pivot->Update(object.position, object.scale + 1.0f);
    }

    if (m_Selected < sceneObjects.size())
    {
        sceneObjects[m_Selected].AABB->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
        sceneObjects[m_Selected].pivot->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    }
}

void SceneEditor::CleanupGeometry()
{
    GeometryFactory::CubeNormals::Destroy();
}

void SceneEditor::AddSceneObject()
{
    float currentTimestamp = (float)glfwGetTime();

    if (currentTimestamp - m_AddObjectLastTime < m_AddObjectCooldown)
        return;

    m_AddObjectLastTime = currentTimestamp;

    // Add Scene Object here

    sceneObjects.push_back({
        glm::mat4(1.0f),
        glm::vec3(0.0f, 0.5f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec4(1.0f),
        false,
        new AABB(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
        new Pivot(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
    });
}

SceneEditor::~SceneEditor()
{
	CleanupGeometry();

    delete m_Raycast;

    for (auto& object : sceneObjects)
    {
        delete object.AABB;
        delete object.pivot;
    }

    sceneObjects.clear();
}
