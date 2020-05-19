#include "SceneEditor.h"

#include "ImGuiWrapper.h"
#include "MousePicker.h"
#include "Block.h"
#include "Sphere.h"
#include "Pyramid.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Ring.h"
#include "Shader.h"
#include "Math.h"

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>


SceneEditor::SceneEditor()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enableSkybox = true;
    sceneSettings.enablePointLights = true;
    sceneSettings.enableSpotLights = true;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(-0.2f, -0.4f, -0.2f);
    sceneSettings.directionalLight.base.ambientIntensity = 0.75f;
    sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;
    sceneSettings.lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 40.0f);

    // point lights
    sceneSettings.pointLights[0].base.enabled = false;
    sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[0].position = glm::vec3(-5.0f, 0.5f, 5.0f);
    sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[0].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[1].base.enabled = false;
    sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[1].position = glm::vec3(5.0f, 0.5f, 5.0f);
    sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[1].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[2].base.enabled = false;
    sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[2].position = glm::vec3(-5.0f, 0.5f, -5.0f);
    sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[2].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[3].base.enabled = false;
    sceneSettings.pointLights[3].base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[3].position = glm::vec3(5.0f, 0.5f, -5.0f);
    sceneSettings.pointLights[3].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[3].base.diffuseIntensity = 1.0f;

    // spot lights
    sceneSettings.spotLights[0].base.base.enabled = false;
    sceneSettings.spotLights[0].base.base.color = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.position = glm::vec3(-5.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[0].direction = glm::vec3(1.0f, -1.0f, 0.0f);
    sceneSettings.spotLights[0].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[0].edge = 0.5f;

    sceneSettings.spotLights[1].base.base.enabled = false;
    sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].base.position = glm::vec3(5.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].direction = glm::vec3(-1.0f, -1.0f, 0.0f);
    sceneSettings.spotLights[1].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[1].edge = 0.5f;

    sceneSettings.spotLights[2].base.base.enabled = false;
    sceneSettings.spotLights[2].base.base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 1.0f, -5.0f);
    sceneSettings.spotLights[2].direction = glm::vec3(0.0f, -1.0f, 1.0f);
    sceneSettings.spotLights[2].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[2].edge = 0.5f;

    sceneSettings.spotLights[3].base.base.enabled = false;
    sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 1.0f, 5.0f);
    sceneSettings.spotLights[3].direction = glm::vec3(0.0f, -1.0f, -1.0f);
    sceneSettings.spotLights[3].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[3].edge = 0.5f;

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_SelectedIndex = 0;
    m_CurrentMeshTypeInt = MESH_TYPE_CUBE;

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    m_Grid = new Grid(10);

    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(40.0f, 40.0f, 40.0f));

    m_Gizmo = new Gizmo();

    m_PositionEdit     = new glm::vec3(0.0f);
    m_RotationEdit     = new glm::vec3(0.0f);
    m_ScaleEdit        = new glm::vec3(1.0f);
    m_ColorEdit        = new glm::vec4(1.0f);
    m_UseTextureEdit   = new bool(false);
    m_TextureNameEdit  = new std::string;
    m_TilingFactorEdit = new float(1.0f);

    // required for directional light enable/disable feature
    m_DirLightEnabledPrev = sceneSettings.directionalLight.base.enabled;
    m_DirLightColorPrev = sceneSettings.directionalLight.base.color;

    m_DisplayLightSources = true;

    m_CurrentSkyboxInt = SKYBOX_DAY;
}

void SceneEditor::SetSkybox()
{
    m_SkyboxFacesDay.push_back("Textures/skybox_4/right.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/left.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/top.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/bottom.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/back.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/front.png");

    m_SkyboxFacesNight.push_back("Textures/skybox_2/right.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/left.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/top.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/bottom.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/back.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/front.png");

    m_SkyboxDay = new Skybox(m_SkyboxFacesDay);
    m_SkyboxNight = new Skybox(m_SkyboxFacesNight);

    m_Skybox = m_SkyboxNight;
}

void SceneEditor::SetTextures()
{
    textures.insert(std::make_pair("plain", new Texture("Textures/plain.png")));
    textures.insert(std::make_pair("semi_transparent", new Texture("Textures/semi_transparent.png")));
    textures.insert(std::make_pair("texture_checker", new Texture("Textures/texture_checker.png")));
    textures.insert(std::make_pair("wood", new Texture("Textures/wood.png")));
    textures.insert(std::make_pair("plank", new Texture("Textures/texture_plank.png")));
    textures.insert(std::make_pair("rock", new Texture("Textures/rock.png")));
    textures.insert(std::make_pair("pyramid", new Texture("Textures/pyramid.png")));
    textures.insert(std::make_pair("lego", new Texture("Textures/lego.png")));
    textures.insert(std::make_pair("marble", new Texture("Textures/marble.jpg")));
    textures.insert(std::make_pair("metal", new Texture("Textures/metal.png")));
    textures.insert(std::make_pair("brick", new Texture("Textures/brick.png")));
    textures.insert(std::make_pair("crate", new Texture("Textures/crate.png")));
    textures.insert(std::make_pair("grass", new Texture("Textures/grass.jpg")));
    textures.insert(std::make_pair("water", new Texture("Textures/water.png")));
    textures.insert(std::make_pair("rock2", new Texture("Textures/rock/Rock-Texture-Surface.jpg")));
    textures.insert(std::make_pair("planet", new Texture("Textures/planet/planet_Quom1200.png")));
    textures.insert(std::make_pair("gold_albedo", new Texture("Textures/PBR/gold/albedo.png")));
    textures.insert(std::make_pair("silver_albedo", new Texture("Textures/PBR/silver/albedo.png")));
    textures.insert(std::make_pair("rusted_iron", new Texture("Textures/PBR/rusted_iron/albedo.png")));
    textures.insert(std::make_pair("grass_albedo", new Texture("Textures/PBR/grass/albedo.png")));
    textures.insert(std::make_pair("wall_albedo", new Texture("Textures/PBR/wall/albedo.png")));
    textures.insert(std::make_pair("plastic_albedo", new Texture("Textures/PBR/plastic/albedo.png")));
}

void SceneEditor::SetupMeshes()
{
}

void SceneEditor::SetupModels()
{
    Sphere* sphere = new Sphere();
    sphere->Create();
    meshes.insert(std::make_pair("sphere", sphere));
}

void SceneEditor::SetGeometry()
{
}

void SceneEditor::Update(float timestep, Window& mainWindow)
{
    m_CurrentTimestamp = timestep;

    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    for (auto& object : m_SceneObjects) {
        object->isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            object->AABB->GetMin(), object->AABB->GetMax(), glm::vec2(0.0f));
        object->mesh->Update(object->scale);
    }

    if (m_CurrentSkyboxInt == SKYBOX_DAY)
        m_Skybox = m_SkyboxDay;
    else if (m_CurrentSkyboxInt == SKYBOX_NIGHT)
        m_Skybox = m_SkyboxNight;

    m_Gizmo->Update(m_Camera->GetPosition(), mainWindow);

    // Switching between scene objects that are currently in focus (mouse over)
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        SelectNextFromMultipleObjects(m_SceneObjects, &m_SelectedIndex);

        if (m_SceneObjects.size() > 0 && m_SceneObjects.at(m_SelectedIndex)->isSelected)
        {
            m_Gizmo->OnMouseClick(mainWindow, m_SceneObjects.at(m_SelectedIndex));
        }
    }

    // Add new scene object with default settings
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1] && mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL])
    {
        AddSceneObject();
    }

    // Copy selected scene object
    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_C])
    {
        CopySceneObject(m_SceneObjects[m_SelectedIndex]);
        m_SceneObjects[m_SelectedIndex]->isSelected = false;
    }

    // Delete selected object
    if (mainWindow.getKeys()[GLFW_KEY_DELETE])
    {
        DeleteSceneObject();
    }

    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_R])
        ResetScene();

    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_S])
        SaveScene();

    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_L])
        LoadScene();

    // Gizmo switching modes
    if (mainWindow.getKeys()[GLFW_KEY_1])
        m_Gizmo->ChangeMode(GIZMO_MODE_TRANSLATE);

    if (mainWindow.getKeys()[GLFW_KEY_2])
        m_Gizmo->ChangeMode(GIZMO_MODE_SCALE);

    if (mainWindow.getKeys()[GLFW_KEY_3])
        m_Gizmo->ChangeMode(GIZMO_MODE_ROTATE);

    if (mainWindow.getKeys()[GLFW_KEY_4])
        m_Gizmo->ChangeMode(GIZMO_MODE_NONE);

}

void SceneEditor::SelectNextFromMultipleObjects(std::vector<SceneObject*> sceneObjects, unsigned int* selected)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectSelect.lastTime < m_ObjectSelect.cooldown) return;
    m_ObjectSelect.lastTime = m_CurrentTimestamp;

    std::vector<unsigned int> sceneObjectsInFocusIndices = std::vector<unsigned int>();

    for (unsigned int i = 0; i < sceneObjects.size(); i++) {
        if (sceneObjects[i]->isSelected) {
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

void SceneEditor::SaveScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneSave.lastTime < m_SceneSave.cooldown) return;
    m_SceneSave.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::SaveScene: Saving %zu objects!\n", m_SceneObjects.size());

    std::vector<std::string> lines;
    for (int i = 0; i < m_SceneObjects.size(); i++)
    {
        lines.push_back("BeginObject\t" + std::to_string(i));
        lines.push_back("Position\t" +
            std::to_string(m_SceneObjects[i]->position.x) + "\t" +
            std::to_string(m_SceneObjects[i]->position.y) + "\t" +
            std::to_string(m_SceneObjects[i]->position.z));
        lines.push_back("Rotation\t" +
            std::to_string(m_SceneObjects[i]->rotation.x) + "\t" +
            std::to_string(m_SceneObjects[i]->rotation.y) + "\t" +
            std::to_string(m_SceneObjects[i]->rotation.z));
        lines.push_back("Scale\t" +
            std::to_string(m_SceneObjects[i]->scale.x) + "\t" +
            std::to_string(m_SceneObjects[i]->scale.y) + "\t" +
            std::to_string(m_SceneObjects[i]->scale.z));
        lines.push_back("Color\t" +
            std::to_string(m_SceneObjects[i]->color.r) + "\t" +
            std::to_string(m_SceneObjects[i]->color.g) + "\t" +
            std::to_string(m_SceneObjects[i]->color.b) + "\t" +
            std::to_string(m_SceneObjects[i]->color.a));
        std::string useTexture = m_SceneObjects[i]->useTexture ? "1" : "0";
        lines.push_back("UseTexture\t" + useTexture);
        lines.push_back("TextureName\t" + m_SceneObjects[i]->textureName);
        lines.push_back("TilingFactor\t" + std::to_string(m_SceneObjects[i]->tilingFactor));
        std::string isSelected = m_SceneObjects[i]->isSelected ? "1" : "0";
        lines.push_back("IsSelected\t" + isSelected);
        lines.push_back("MeshType\t" + std::to_string(m_SceneObjects[i]->meshType));
        lines.push_back("EndObject");
    }

    std::ofstream ofs;
    ofs.open(m_SceneFilename);
    for (auto& line : lines)
    {
        ofs << line << std::endl;
        // printf("%s\n", line.c_str());
    }
    ofs.close();

    printf("SceneEditor::SaveScene: Content saved to file '%s'\n", m_SceneFilename);
}

void SceneEditor::LoadScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneLoad.lastTime < m_SceneLoad.cooldown) return;
    m_SceneLoad.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::LoadScene: Loading objects...\n");

    std::string sceneFileContent = Shader::ReadFile(m_SceneFilename);

    // printf(sceneFileContent.c_str());

    std::vector<std::string> lines;
    std::istringstream iss(sceneFileContent);
    std::string tmpLine;
    while (std::getline(iss, tmpLine, '\n')) {
        lines.push_back(tmpLine);
    }

    unsigned int objectId;
    SceneObject sceneObject;
    m_SceneObjects.clear();

    for (auto& line : lines)
    {
        // printf("%s\n", line.c_str());
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string tmpToken;
        while (std::getline(iss, tmpToken, '\t')) {
            tokens.push_back(tmpToken);
        }

        // for (auto& token : tokens)
        //     printf("%s\n", token.c_str());
        
        if (tokens.size() >= 2 && tokens[0] == "BeginObject") {
            objectId = (unsigned int)std::stoi(tokens[1]);
            sceneObject = {};
            // printf("ObjectID=%i\n", objectId);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Position") {
            sceneObject.position = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Position %.2ff %.2ff %.2ff\n", sceneObject.position.x, sceneObject.position.y, sceneObject.position.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Rotation") {
            sceneObject.rotation = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Rotation %.2ff %.2ff %.2ff\n", sceneObject.rotation.x, sceneObject.rotation.y, sceneObject.rotation.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Scale") {
            sceneObject.scale = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Scale %.2ff %.2ff %.2ff\n", sceneObject.scale.x, sceneObject.scale.y, sceneObject.scale.z);
        }
        else if (tokens.size() >= 5 && tokens[0] == "Color") {
            sceneObject.color = glm::vec4(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[4]));
            // printf("Color %.2ff %.2ff %.2ff %.2ff\n", sceneObject.color.r, sceneObject.color.g, sceneObject.color.b, sceneObject.color.a);
        }
        else if (tokens.size() >= 2 && tokens[0] == "UseTexture") {
            sceneObject.useTexture = std::stoi(tokens[1]) == 1 ? true : false;
            // printf("UseTexture %d\n", sceneObject.useTexture);
        }
        else if (tokens.size() >= 2 && tokens[0] == "TextureName") {
            sceneObject.textureName = tokens[1];
            // printf("UseTexture %s\n", sceneObject.textureName.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "TilingFactor") {
            sceneObject.tilingFactor = std::stof(tokens[1]);
            // printf("TilingFactor %.2f\n", sceneObject.tilingFactor);
        }
        else if (tokens.size() >= 2 && tokens[0] == "IsSelected") {
            sceneObject.isSelected = std::stoi(tokens[1]) == 1 ? true : false;
            if (sceneObject.isSelected) m_SelectedIndex = (unsigned int)m_SceneObjects.size();
            // printf("IsSelected %d\n", sceneObject.isSelected);
        }
        else if (tokens.size() >= 2 && tokens[0] == "MeshType") {
            sceneObject.meshType = std::stoi(tokens[1]);
            // printf("MeshType %d\n", sceneObject.meshType);
        }
        else if (tokens.size() >= 1 && tokens[0] == "EndObject") {
            sceneObject.id = (int)m_SceneObjects.size();
            sceneObject.transform = Math::CreateTransform(sceneObject.position, sceneObject.rotation, sceneObject.scale);
            sceneObject.AABB  = new AABB(sceneObject.position, sceneObject.rotation, sceneObject.scale);
            sceneObject.pivot = new Pivot(sceneObject.position, sceneObject.scale);
            sceneObject.mesh  = CreateNewPrimitive(sceneObject.meshType, sceneObject.scale);
            m_SceneObjects.push_back(&sceneObject);
            // printf("EndObject: New SceneObject added to m_SceneObjects...\n");
        }
    }

    if (m_SceneObjects.size() > 0)
        m_Gizmo->SetSceneObject(m_SceneObjects[m_SceneObjects.size() - 1]);
}

void SceneEditor::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    MousePicker* mp = MousePicker::Get();

#if 0
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
#endif

    if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
    {
        m_PositionEdit =     &m_SceneObjects[m_SelectedIndex]->position;
        m_RotationEdit =     &m_SceneObjects[m_SelectedIndex]->rotation;
        m_ScaleEdit =        &m_SceneObjects[m_SelectedIndex]->scale;
        m_ColorEdit =        &m_SceneObjects[m_SelectedIndex]->color;
        m_UseTextureEdit =   &m_SceneObjects[m_SelectedIndex]->useTexture;
        m_TextureNameEdit  = &m_SceneObjects[m_SelectedIndex]->textureName;
        m_TilingFactorEdit = &m_SceneObjects[m_SelectedIndex]->tilingFactor;
    }

    ImGui::Begin("Transform");

    ImGui::SliderFloat3("Position", (float*)m_PositionEdit, -10.0f, 10.0f);
    ImGui::SliderFloat3("Rotation", (float*)m_RotationEdit, -179.0f, 180.0f);
    ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.1f, 20.0f);
    ImGui::ColorEdit4("Color", (float*)m_ColorEdit);
    ImGui::Checkbox("Use Texture", m_UseTextureEdit);

    // Begin ImGui drop-down list
    std::vector<const char*> items;
    std::map<std::string, Texture*>::iterator it;
    for (it = textures.begin(); it != textures.end(); it++)
        items.push_back(it->first.c_str());
    static const char* currentItem = m_TextureNameEdit->c_str();

    if (ImGui::BeginCombo("Texture Name", currentItem))
    {
        for (int n = 0; n < items.size(); n++)
        {
            bool isSelected = (currentItem == items[n]);
            if (ImGui::Selectable(items[n], isSelected))
            {
                currentItem = items[n];
                if (m_SelectedIndex < m_SceneObjects.size())
                    m_SceneObjects[m_SelectedIndex]->textureName = items[n];
                else
                    m_SelectedIndex = m_SceneObjects.size() > 0 ? (unsigned int)m_SceneObjects.size() - 1 : 0;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    // End ImGui drop-down list

    ImGui::SliderFloat("Tiling Factor", m_TilingFactorEdit, 0.0f, 10.0f);
    ImGui::SliderInt("Selected Object", (int*)&m_SelectedIndex, 0, (int)(m_SceneObjects.size() - 1));

    bool gizmoActive = m_Gizmo->GetActive();
    int sceneObjectCount = (int)m_SceneObjects.size();
    Bool3 axesEnabled = m_Gizmo->GetAxesEnabled();

    ImGui::Separator();
    ImGui::Text("Transform Gizmo");
    ImGui::Text(m_Gizmo->GetModeDescriptive().c_str());
    ImGui::SliderInt("Scene Objects Count", &sceneObjectCount, 0, 100);
    ImGui::Checkbox("Gizmo Active", &gizmoActive);
    ImGui::Text("Axes Enabled");
    ImGui::Checkbox("Axis X", &axesEnabled.x);
    ImGui::Checkbox("Axis Y", &axesEnabled.y);
    ImGui::Checkbox("Axis Z", &axesEnabled.z);

    ImGui::Separator();
    ImGui::Text("Select Object Type");
    ImGui::RadioButton("Cube",     &m_CurrentMeshTypeInt, MESH_TYPE_CUBE);
    ImGui::RadioButton("Pyramid",  &m_CurrentMeshTypeInt, MESH_TYPE_PYRAMID);
    ImGui::RadioButton("Sphere",   &m_CurrentMeshTypeInt, MESH_TYPE_SPHERE);
    ImGui::RadioButton("Cylinder", &m_CurrentMeshTypeInt, MESH_TYPE_CYLINDER);
    ImGui::RadioButton("Cone",     &m_CurrentMeshTypeInt, MESH_TYPE_CONE);
    ImGui::RadioButton("Ring",     &m_CurrentMeshTypeInt, MESH_TYPE_RING);

    ImGui::Separator();
    ImGui::Text("Select Skybox");
    ImGui::RadioButton("Day",   &m_CurrentSkyboxInt, SKYBOX_DAY);
    ImGui::RadioButton("Night", &m_CurrentSkyboxInt, SKYBOX_NIGHT);

    ImGui::Separator();
    float FOV = GetFOV();
    ImGui::SliderFloat("FOV", &FOV, 1.0f, 120.0f);
    SetFOV(FOV);

    ImGui::Separator();
    ImGui::Text("Lights");
    ImGui::Checkbox("Display Light Sources", &m_DisplayLightSources);

    SDirectionalLight directionalLight;

    directionalLight.base.enabled          = m_LightManager->directionalLight.GetEnabled();
    directionalLight.base.color            = m_LightManager->directionalLight.GetColor();
    directionalLight.base.ambientIntensity = m_LightManager->directionalLight.GetAmbientIntensity();
    directionalLight.base.diffuseIntensity = m_LightManager->directionalLight.GetDiffuseIntensity();
    directionalLight.direction             = m_LightManager->directionalLight.GetDirection();

    ImGui::Separator();
    ImGui::Text("Directional Light");
    ImGui::Checkbox(    "DL Enabled", &directionalLight.base.enabled);
    ImGui::ColorEdit3(  "DL Color", glm::value_ptr(directionalLight.base.color));
    ImGui::SliderFloat3("DL Direction", glm::value_ptr(directionalLight.direction), -1.0f, 1.0f);
    ImGui::SliderFloat( "DL Ambient Intensity", &directionalLight.base.ambientIntensity, 0.0f, 4.0f);
    ImGui::SliderFloat( "DL Diffuse Intensity", &directionalLight.base.diffuseIntensity, 0.0f, 4.0f);

    // Shutdown directional light (it appears it's better to do it here than in shader
    if (directionalLight.base.enabled != m_DirLightEnabledPrev)
    {
        if (directionalLight.base.enabled)
            directionalLight.base.color = m_DirLightColorPrev;
        else
            directionalLight.base.color = glm::vec3(0.0f, 0.0f, 0.0f);

        m_DirLightEnabledPrev = directionalLight.base.enabled;
    }

    m_LightManager->directionalLight.SetEnabled(directionalLight.base.enabled);
    m_LightManager->directionalLight.SetColor(directionalLight.base.color);
    m_LightManager->directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
    m_LightManager->directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);
    m_LightManager->directionalLight.SetDirection(directionalLight.direction);

    // Point Lights
    ImGui::Separator();
    ImGui::Text("Point Lights");
    ImGui::Separator();

    SPointLight pointLights[4];
    char locBuff[100] = { '\0' };
    for (unsigned int pl = 0; pl < m_LightManager->pointLightCount; pl++)
    {
        pointLights[pl].base.enabled          = m_LightManager->pointLights[pl].GetEnabled();
        pointLights[pl].base.color            = m_LightManager->pointLights[pl].GetColor();
        pointLights[pl].base.ambientIntensity = m_LightManager->pointLights[pl].GetAmbientIntensity();
        pointLights[pl].base.diffuseIntensity = m_LightManager->pointLights[pl].GetDiffuseIntensity();
        pointLights[pl].position              = m_LightManager->pointLights[pl].GetPosition();
        pointLights[pl].constant              = m_LightManager->pointLights[pl].GetConstant();
        pointLights[pl].linear                = m_LightManager->pointLights[pl].GetLinear();
        pointLights[pl].exponent              = m_LightManager->pointLights[pl].GetExponent();

        snprintf(locBuff, sizeof(locBuff), "Point Light %i", pl);
        ImGui::Text(locBuff);

        snprintf(locBuff, sizeof(locBuff), "PL %i Enabled", pl);
        ImGui::Checkbox(locBuff, &pointLights[pl].base.enabled);
        snprintf(locBuff, sizeof(locBuff), "PL %i Color", pl);
        ImGui::ColorEdit3(locBuff, glm::value_ptr(pointLights[pl].base.color));
        snprintf(locBuff, sizeof(locBuff), "PL %i Position", pl);
        ImGui::SliderFloat3(locBuff, glm::value_ptr(pointLights[pl].position), -20.0f, 20.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Ambient Intensity", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].base.ambientIntensity, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Diffuse Intensity", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].base.diffuseIntensity, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Constant", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].constant, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Linear", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].linear, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Exponent", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].exponent, -2.0f, 2.0f);

        m_LightManager->pointLights[pl].SetEnabled(pointLights[pl].base.enabled);
        m_LightManager->pointLights[pl].SetColor(pointLights[pl].base.color);
        m_LightManager->pointLights[pl].SetAmbientIntensity(pointLights[pl].base.ambientIntensity);
        m_LightManager->pointLights[pl].SetDiffuseIntensity(pointLights[pl].base.diffuseIntensity);
        m_LightManager->pointLights[pl].SetPosition(pointLights[pl].position);
        m_LightManager->pointLights[pl].SetConstant(pointLights[pl].constant);
        m_LightManager->pointLights[pl].SetLinear(pointLights[pl].linear);
        m_LightManager->pointLights[pl].SetExponent(pointLights[pl].exponent);

        ImGui::Separator();
    }

    ImGui::Separator();
    ImGui::Text("Spot Lights");
    ImGui::Separator();

    SSpotLight spotLights[4];

    for (unsigned int sl = 0; sl < m_LightManager->spotLightCount; sl++)
    {
        spotLights[sl].base.base.enabled          = m_LightManager->spotLights[sl].GetBasePL()->GetEnabled();
        spotLights[sl].base.base.color            = m_LightManager->spotLights[sl].GetBasePL()->GetColor();
        spotLights[sl].base.base.ambientIntensity = m_LightManager->spotLights[sl].GetBasePL()->GetAmbientIntensity();
        spotLights[sl].base.base.diffuseIntensity = m_LightManager->spotLights[sl].GetBasePL()->GetDiffuseIntensity();
        spotLights[sl].base.position              = m_LightManager->spotLights[sl].GetBasePL()->GetPosition();
        spotLights[sl].base.constant              = m_LightManager->spotLights[sl].GetBasePL()->GetConstant();
        spotLights[sl].base.linear                = m_LightManager->spotLights[sl].GetBasePL()->GetLinear();
        spotLights[sl].base.exponent              = m_LightManager->spotLights[sl].GetBasePL()->GetExponent();
        spotLights[sl].direction                  = m_LightManager->spotLights[sl].GetDirection();
        spotLights[sl].edge                       = m_LightManager->spotLights[sl].GetEdge();

        snprintf(locBuff, sizeof(locBuff), "Spot Light %i", sl);
        ImGui::Text(locBuff);

        snprintf(locBuff, sizeof(locBuff), "SL %i Enabled", sl);
        ImGui::Checkbox(locBuff, &spotLights[sl].base.base.enabled);
        snprintf(locBuff, sizeof(locBuff), "SL %i Color", sl);
        ImGui::ColorEdit3(locBuff, glm::value_ptr(spotLights[sl].base.base.color));
        snprintf(locBuff, sizeof(locBuff), "SL %i Position", sl);
        ImGui::SliderFloat3(locBuff, glm::value_ptr(spotLights[sl].base.position), -20.0f, 20.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Direction", sl);
        ImGui::SliderFloat3(locBuff, glm::value_ptr(spotLights[sl].direction), -1.0f, 1.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Edge", sl);
        ImGui::SliderFloat(locBuff, &spotLights[sl].edge, -100.0f, 100.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Ambient Intensity", sl);
        ImGui::SliderFloat(locBuff, &spotLights[sl].base.base.ambientIntensity, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Diffuse Intensity", sl);
        ImGui::SliderFloat(locBuff, &spotLights[sl].base.base.diffuseIntensity, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Constant", sl);
        ImGui::SliderFloat(locBuff, &spotLights[sl].base.constant, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Linear", sl);
        ImGui::SliderFloat(locBuff, &spotLights[sl].base.linear, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "SL %i Exponent", sl);
        ImGui::SliderFloat(locBuff, &spotLights[sl].base.exponent, -2.0f, 2.0f);

        m_LightManager->spotLights[sl].GetBasePL()->SetEnabled(spotLights[sl].base.base.enabled);
        m_LightManager->spotLights[sl].GetBasePL()->SetColor(spotLights[sl].base.base.color);
        m_LightManager->spotLights[sl].GetBasePL()->SetAmbientIntensity(spotLights[sl].base.base.ambientIntensity);
        m_LightManager->spotLights[sl].GetBasePL()->SetDiffuseIntensity(spotLights[sl].base.base.diffuseIntensity);
        m_LightManager->spotLights[sl].GetBasePL()->SetPosition(spotLights[sl].base.position);
        m_LightManager->spotLights[sl].GetBasePL()->SetConstant(spotLights[sl].base.constant);
        m_LightManager->spotLights[sl].GetBasePL()->SetLinear(spotLights[sl].base.linear);
        m_LightManager->spotLights[sl].GetBasePL()->SetExponent(spotLights[sl].base.exponent);
        m_LightManager->spotLights[sl].SetDirection(spotLights[sl].direction);
        m_LightManager->spotLights[sl].SetEdge(spotLights[sl].edge);

        ImGui::Separator();
    }
    ImGui::End();
}

void SceneEditor::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    Shader* shaderEditor = shaders["editor_object"];
    shaderEditor->Bind();

    for (auto& object : m_SceneObjects)
    {
        object->transform = Math::CreateTransform(object->position, object->rotation, glm::vec3(1.0f));

        // For meshes that can't be scaled on vertex level
        if (object->meshType == MESH_TYPE_RING)
            object->transform = glm::scale(object->transform, object->scale);

        shaderEditor->setMat4("model", object->transform);
        shaderEditor->setVec4("tintColor", object->color);
        shaderEditor->setBool("isSelected", object->isSelected);

        if (object->useTexture && object->textureName != "")
            textures[object->textureName]->Bind(0);
        else
            textures["plain"]->Bind(0);
        shaderEditor->setInt("albedoMap", 0);
        shaderEditor->setFloat("tilingFactor", object->tilingFactor);

        object->mesh->Render();

        object->AABB->Update(object->position, object->rotation, object->scale);
        object->pivot->Update(object->position, object->scale + 1.0f);
    }

    // Render spheres on light positions
    // Directional light (somewhere on pozitive Y axis, at X=0, Z=0)
    // Render Sphere (Light source)
    glm::mat4 model;

    textures["plain"]->Bind(0);
    textures["plain"]->Bind(1);
    textures["plain"]->Bind(2);

    shaderEditor->setVec4("tintColor", glm::vec4(1.0f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 20.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    shaderEditor->setMat4("model", model);
    if (m_DisplayLightSources)
        meshes["sphere"]->Render();

    // Point lights
    for (unsigned int i = 0; i < m_LightManager->pointLightCount; i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_LightManager->pointLights[i].GetPosition());
        model = glm::scale(model, glm::vec3(0.25f));
        shaderEditor->setMat4("model", model);
        if (m_DisplayLightSources && m_LightManager->pointLights[i].GetEnabled())
            meshes["sphere"]->Render();
    }

    // Spot lights
    for (unsigned int i = 0; i < m_LightManager->spotLightCount; i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_LightManager->spotLights[i].GetBasePL()->GetPosition());
        model = glm::scale(model, glm::vec3(0.25f));
        shaderEditor->setMat4("model", model);
        if (m_DisplayLightSources && m_LightManager->spotLights[i].GetBasePL()->GetEnabled())
            meshes["sphere"]->Render();
    }

    if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
        m_Gizmo->Render(shaderEditor);

    /* End of shaderEditor */

    /* Begin of shaderBasic */
    if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
    {
        shaders["basic"]->Bind();
        shaders["basic"]->setMat4("model", glm::mat4(1.0f));
        shaders["basic"]->setMat4("view", m_Camera->CalculateViewMatrix());
        shaders["basic"]->setMat4("projection", projectionMatrix);

        m_SceneObjects[m_SelectedIndex]->AABB->Draw();
        m_SceneObjects[m_SelectedIndex]->pivot->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    }

    m_Grid->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    m_PivotScene->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
}

void SceneEditor::CleanupGeometry()
{
}

void SceneEditor::AddSceneObject()
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectAdd.lastTime < m_ObjectAdd.cooldown) return;
    m_ObjectAdd.lastTime = m_CurrentTimestamp;

    m_Gizmo->SetActive(false);

    Mesh* mesh = CreateNewPrimitive(m_CurrentMeshTypeInt, glm::vec3(1.0f));

    // Add Scene Object here
    SceneObject* sceneObject = new SceneObject{
        (int)m_SceneObjects.size(),
        glm::mat4(1.0f),
        defaultSpawnPosition,
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec4(1.0f),
        false,
        "plain",
        1.0f,
        true,
        new AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)),
        new Pivot(glm::vec3(0.0f), glm::vec3(1.0f)),
        mesh,
        m_CurrentMeshTypeInt,
    };

    m_SceneObjects.push_back(sceneObject);
    m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;
}

void SceneEditor::CopySceneObject(SceneObject* sceneObject)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectCopy.lastTime < m_ObjectCopy.cooldown) return;
    m_ObjectCopy.lastTime = m_CurrentTimestamp;

    m_Gizmo->SetActive(false);

    Mesh* newMesh = CreateNewPrimitive(sceneObject->meshType, sceneObject->mesh->GetScale());

    sceneObject->id = (int)m_SceneObjects.size(),
    sceneObject->isSelected = true;
    sceneObject->AABB = new AABB(sceneObject->position, sceneObject->rotation, sceneObject->scale);
    sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);
    sceneObject->mesh = newMesh;

    m_SceneObjects.push_back(sceneObject);
}

void SceneEditor::DeleteSceneObject()
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectDelete.lastTime < m_ObjectDelete.cooldown) return;
    m_ObjectDelete.lastTime = m_CurrentTimestamp;

    delete m_SceneObjects[m_SelectedIndex]->AABB;
    delete m_SceneObjects[m_SelectedIndex]->pivot;
    delete m_SceneObjects[m_SelectedIndex]->mesh;

    if (m_SelectedIndex < m_SceneObjects.size())
        m_SceneObjects.erase(m_SceneObjects.begin() + m_SelectedIndex);

    if (m_SceneObjects.size() > 0) m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;

    // refresh scene object IDs
    for (int i = 0; i < m_SceneObjects.size(); i++)
        m_SceneObjects[i]->id = i;
}

Mesh* SceneEditor::CreateNewPrimitive(int meshTypeID, glm::vec3 scale)
{
    Mesh* mesh;
    switch (meshTypeID)
    {
    case MESH_TYPE_CUBE:
        mesh = new Block(scale);
        break;
    case MESH_TYPE_PYRAMID:
        mesh = new Pyramid(scale);
        break;
    case MESH_TYPE_SPHERE:
        mesh = new Sphere(scale);
        break;
    case MESH_TYPE_CYLINDER:
        mesh = new Cylinder(scale);
        break;
    case MESH_TYPE_CONE:
        mesh = new Cone(scale);
        break;
    case MESH_TYPE_RING:
        mesh = new Ring(scale);
        break;
    default:
        mesh = new Block(scale);
        break;
    }
    return mesh;
}

void SceneEditor::ResetScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneReset.lastTime < m_SceneReset.cooldown) return;
    m_SceneReset.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::ResetScene: Deleting %zu objects!\n", m_SceneObjects.size());

    for (auto& object : m_SceneObjects)
    {
        // delete object->AABB;
        // delete object->pivot;
        // delete object->mesh;
    }
    m_SceneObjects.clear();
}

SceneEditor::~SceneEditor()
{
    SaveScene();

	CleanupGeometry();

    delete m_PivotScene;
    delete m_Grid;
    delete m_Raycast;
    delete m_Gizmo;

    ResetScene();
}
