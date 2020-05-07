#include "SceneEditor.h"

#include "ImGuiWrapper.h"
#include "MousePicker.h"
#include "Block.h"

#include <vector>
#include <map>
#include <string>


SceneEditor::SceneEditor()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enableSkybox = true;

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_SelectedIndex = 0;

    sceneObjects.resize(2);

    sceneObjects[0].position = glm::vec3(2.0f, 0.5f, 2.0f);
    sceneObjects[1].position = glm::vec3(-2.0f, 0.5f, -2.0f);

    sceneObjects[0].rotation = glm::vec3(0.0f);
    sceneObjects[1].rotation = glm::vec3(0.0f);

    sceneObjects[0].scale = glm::vec3(1.0f);
    sceneObjects[1].scale = glm::vec3(1.0f);

    sceneObjects[0].color = glm::vec4(0.8f, 0.4f, 0.0f, 0.8f);
    sceneObjects[1].color = glm::vec4(0.4f, 0.4f, 0.8f, 0.8f);

    sceneObjects[0].useTexture = false;
    sceneObjects[1].useTexture = false;

    sceneObjects[0].tilingFactor = 1.0f;
    sceneObjects[1].tilingFactor = 1.0f;

    sceneObjects[0].AABB = new AABB(sceneObjects[0].position, sceneObjects[0].scale);
    sceneObjects[1].AABB = new AABB(sceneObjects[1].position, sceneObjects[1].scale);

    sceneObjects[0].pivot = new Pivot(sceneObjects[0].position, sceneObjects[0].scale + 1.0f);
    sceneObjects[1].pivot = new Pivot(sceneObjects[1].position, sceneObjects[1].scale + 1.0f);

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    m_Grid = new Grid(10);
}

void SceneEditor::SetSkybox()
{
    skyboxFaces.push_back("Textures/skybox_4/right.png");
    skyboxFaces.push_back("Textures/skybox_4/left.png");
    skyboxFaces.push_back("Textures/skybox_4/top.png");
    skyboxFaces.push_back("Textures/skybox_4/bottom.png");
    skyboxFaces.push_back("Textures/skybox_4/back.png");
    skyboxFaces.push_back("Textures/skybox_4/front.png");
    m_Skybox = new Skybox(skyboxFaces);
}

void SceneEditor::SetTextures()
{
    textures.insert(std::make_pair("semi_transparent", new Texture("Textures/semi_transparent.png")));
    textures.insert(std::make_pair("texture_checker", new Texture("Textures/texture_checker.png")));
    textures.insert(std::make_pair("plain", new Texture("Textures/plain.png")));
}

void SceneEditor::SetupMeshes()
{
    m_Quad = new Quad();
    meshes.insert(std::make_pair("quad", m_Quad));

    Block* m_Block = new Block(1.0f, 1.0f, 1.0f);
    meshes.insert(std::make_pair("block", m_Block));
}

void SceneEditor::SetupModels()
{
}

void SceneEditor::SetGeometry()
{
}

void SceneEditor::Update(float timestep, Window& mainWindow)
{
    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    for (auto& object : sceneObjects) {
        object.isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            object.AABB->GetMin(), object.AABB->GetMax(), glm::vec2(0.0f));
    }

    // Switching between scene objects that are currently in focus (mouse over)
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        SelectNextFromMultipleObjects(sceneObjects, &m_SelectedIndex);
    }

    // Add new scene object with default settings
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1] && mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL])
    {
        AddSceneObject();
    }

    // Copy selected scene object
    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_C])
    {
        CopySceneObject(sceneObjects[m_SelectedIndex]);
        sceneObjects[m_SelectedIndex].isSelected = false;
    }

    // Delete selected object
    if (mainWindow.getKeys()[GLFW_KEY_DELETE])
    {
        DeleteSceneObject();
    }
}

void SceneEditor::SelectNextFromMultipleObjects(std::vector<SceneObject> sceneObjects, unsigned int* selected)
{
    // Cooldown
    float currentTimestamp = (float)glfwGetTime();
    if (currentTimestamp - m_ObjectSelect.lastTime < m_ObjectSelect.cooldown) return;
    m_ObjectSelect.lastTime = currentTimestamp;

    std::vector<unsigned int> sceneObjectsInFocusIndices = std::vector<unsigned int>();

    for (unsigned int i = 0; i < sceneObjects.size(); i++) {
        if (sceneObjects[i].isSelected) {
            *selected = i;
            sceneObjectsInFocusIndices.push_back(i);
        }
    }

    // if there is 0 or 1 elements in focus - finish
    if (sceneObjectsInFocusIndices.size() <= 1) return;

    // handle multiple selections
    m_ObjectInFocusPrev++;
    if (m_ObjectInFocusPrev > sceneObjectsInFocusIndices.size() - 1)
        m_ObjectInFocusPrev = 0;
    *selected = sceneObjectsInFocusIndices[m_ObjectInFocusPrev];

    // printf("Total objects in focus: %i, currently selected: %i\n", (int)sceneObjectsInFocusIndices.size(), (int)currentlySelected);
}

bool SceneEditor::IsObjectSelected(unsigned int objectIndex)
{
    return objectIndex = m_SelectedIndex;
}

void SceneEditor::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    MousePicker* mp = MousePicker::Get();

    ImGui::Begin("Mouse Picker Info");

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
    ImGui::End();

    if (m_SelectedIndex < sceneObjects.size())
    {
        m_PositionEdit =     &sceneObjects[m_SelectedIndex].position;
        m_RotationEdit =     &sceneObjects[m_SelectedIndex].rotation;
        m_ScaleEdit =        &sceneObjects[m_SelectedIndex].scale;
        m_ColorEdit =        &sceneObjects[m_SelectedIndex].color;
        m_UseTextureEdit =   &sceneObjects[m_SelectedIndex].useTexture;
        m_TilingFactorEdit = &sceneObjects[m_SelectedIndex].tilingFactor;
    }

    ImGui::Begin("Transform");

    ImGui::SliderFloat3("Position", (float*)m_PositionEdit, -10.0f, 10.0f);
    // ImGui::SliderFloat3("Rotation", (float*)m_RotationEdit, -179.0f, 180.0f);
    ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.1f, 20.0f);
    ImGui::ColorEdit4("Color", (float*)m_ColorEdit);
    ImGui::Checkbox("Use Texture", m_UseTextureEdit);
    ImGui::SliderFloat("Tiling Factor", m_TilingFactorEdit, 0.0f, 10.0f);
    ImGui::SliderInt("Selected Object", (int*)&m_SelectedIndex, 0, (int)(sceneObjects.size() - 1));

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

        if (object.useTexture)
            textures["texture_checker"]->Bind(0);
        else
            textures["plain"]->Bind(0);
        shaders["editor_object"]->setInt("albedoMap", 0);
        shaders["editor_object"]->setFloat("tilingFactor", object.tilingFactor);

        meshes["block"]->Render();

        object.AABB->Update(object.position, object.scale);
        object.pivot->Update(object.position, object.scale + 1.0f);
    }

    if (m_SelectedIndex < sceneObjects.size())
    {
        sceneObjects[m_SelectedIndex].AABB->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
        sceneObjects[m_SelectedIndex].pivot->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    }

    m_Grid->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
}

void SceneEditor::CleanupGeometry()
{
}

void SceneEditor::AddSceneObject()
{
    // Cooldown
    float currentTimestamp = (float)glfwGetTime();
    if (currentTimestamp - m_ObjectAdd.lastTime < m_ObjectAdd.cooldown) return;
    m_ObjectAdd.lastTime = currentTimestamp;

    // Add Scene Object here
    SceneObject sceneObject = {
        glm::mat4(1.0f),
        defaultSpawnPosition,
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec4(1.0f),
        false,
        1.0f,
        true,
        new AABB(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
        new Pivot(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
    };

    sceneObjects.push_back(sceneObject);
    m_SelectedIndex++;
}

void SceneEditor::CopySceneObject(SceneObject sceneObject)
{
    // Cooldown
    float currentTimestamp = (float)glfwGetTime();
    if (currentTimestamp - m_ObjectCopy.lastTime < m_ObjectCopy.cooldown) return;
    m_ObjectCopy.lastTime = currentTimestamp;

    sceneObject.isSelected = true;
    sceneObject.AABB = new AABB(sceneObject.position, sceneObject.scale);
    sceneObject.pivot = new Pivot(sceneObject.position, sceneObject.scale);

    sceneObjects.push_back(sceneObject);
}

void SceneEditor::DeleteSceneObject()
{
    // Cooldown
    float currentTimestamp = (float)glfwGetTime();
    if (currentTimestamp - m_ObjectDelete.lastTime < m_ObjectDelete.cooldown) return;
    m_ObjectDelete.lastTime = currentTimestamp;

    if (m_SelectedIndex < sceneObjects.size())
        sceneObjects.erase(sceneObjects.begin() + m_SelectedIndex);

    if (m_SelectedIndex > 0) m_SelectedIndex--;
}

SceneEditor::~SceneEditor()
{
	CleanupGeometry();

    delete m_Grid;
    delete m_Raycast;

    for (auto& object : sceneObjects)
    {
        delete object.AABB;
        delete object.pivot;
    }

    sceneObjects.clear();
}
