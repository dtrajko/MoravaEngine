#include "SceneEditor.h"

#include "ImGuiWrapper.h"
#include "MousePicker.h"
#include "Block.h"
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
    sceneSettings.lightDirection = glm::vec3(0.5f, -0.7f, -0.5f);
    sceneSettings.ambientIntensity = 1.0f;
    sceneSettings.diffuseIntensity = 0.0f;
    sceneSettings.lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 40.0f);

    // point lights
    sceneSettings.pLight_0_color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.pLight_0_position = glm::vec3(-5.0f, 2.0f, 5.0f);
    sceneSettings.pLight_0_diffuseIntensity;
    sceneSettings.pLight_1_color = glm::vec3(1.0f, 0.0f, 1.0f);
    sceneSettings.pLight_1_position = glm::vec3(5.0f, 2.0f, 5.0f);
    sceneSettings.pLight_1_diffuseIntensity;
    sceneSettings.pLight_2_color = glm::vec3(0.0f, 1.0f, 1.0f);
    sceneSettings.pLight_2_position = glm::vec3(-5.0f, 2.0f, -5.0f);
    sceneSettings.pLight_2_diffuseIntensity;
    sceneSettings.pLight_3_color = glm::vec3(0.0f, 0.0f, 1.0f);
    sceneSettings.pLight_3_position = glm::vec3(5.0f, 2.0f, -5.0f);
    sceneSettings.pLight_3_diffuseIntensity;

    // spot lights
    sceneSettings.sLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.sLight_0_position = glm::vec3(-5.0f, 1.0f,  0.0f);
    sceneSettings.sLight_0_direction = glm::vec3(0.1f, -0.8f, 0.1f);
    sceneSettings.sLight_1_color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.sLight_1_position = glm::vec3( 5.0f, 1.0f,  0.0f);
    sceneSettings.sLight_1_direction = glm::vec3(0.1f, -0.8f, 0.1f);
    sceneSettings.sLight_2_color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.sLight_2_position = glm::vec3( 0.0f, 1.0f, -5.0f);
    sceneSettings.sLight_2_direction = glm::vec3(0.1f, -0.8f, 0.1f);
    sceneSettings.sLight_3_color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.sLight_3_position = glm::vec3( 0.0f, 1.0f,  5.0f);
    sceneSettings.sLight_3_direction = glm::vec3(0.1f, -0.8f, 0.1f);

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_SelectedIndex = 0;

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    m_Grid = new Grid(10);

    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(40.0f, 40.0f, 40.0f));

    m_PositionEdit     = new glm::vec3(0.0f);
    m_RotationEdit     = new glm::vec3(0.0f);
    m_ScaleEdit        = new glm::vec3(1.0f);
    m_ColorEdit        = new glm::vec4(1.0f);
    m_UseTextureEdit   = new bool(false);
    m_TextureNameEdit  = new std::string;
    m_TilingFactorEdit = new float(1.0f);
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
}

void SceneEditor::SetGeometry()
{
}

void SceneEditor::Update(float timestep, Window& mainWindow)
{
    m_CurrentTimestamp = timestep;

    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    for (auto& object : m_SceneObjects) {
        object.isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            object.AABB->GetMin(), object.AABB->GetMax(), glm::vec2(0.0f));

        Block* block = (Block*)object.mesh;
        block->Update(object.scale);
    }

    // Switching between scene objects that are currently in focus (mouse over)
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        SelectNextFromMultipleObjects(m_SceneObjects, &m_SelectedIndex);
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
        m_SceneObjects[m_SelectedIndex].isSelected = false;
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
}

void SceneEditor::SelectNextFromMultipleObjects(std::vector<SceneObject> sceneObjects, unsigned int* selected)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectSelect.lastTime < m_ObjectSelect.cooldown) return;
    m_ObjectSelect.lastTime = m_CurrentTimestamp;

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
            std::to_string(m_SceneObjects[i].position.x) + "\t" +
            std::to_string(m_SceneObjects[i].position.y) + "\t" +
            std::to_string(m_SceneObjects[i].position.z) + "\t");
        lines.push_back("Rotation\t" +
            std::to_string(m_SceneObjects[i].rotation.x) + "\t" +
            std::to_string(m_SceneObjects[i].rotation.y) + "\t" +
            std::to_string(m_SceneObjects[i].rotation.z) + "\t");
        lines.push_back("Scale\t" +
            std::to_string(m_SceneObjects[i].scale.x) + "\t" +
            std::to_string(m_SceneObjects[i].scale.y) + "\t" +
            std::to_string(m_SceneObjects[i].scale.z) + "\t");
        lines.push_back("Color\t" +
            std::to_string(m_SceneObjects[i].color.r) + "\t" +
            std::to_string(m_SceneObjects[i].color.g) + "\t" +
            std::to_string(m_SceneObjects[i].color.b) + "\t" +
            std::to_string(m_SceneObjects[i].color.a) + "\t");
        std::string useTexture = m_SceneObjects[i].useTexture ? "1" : "0";
        lines.push_back("UseTexture\t" + useTexture);
        lines.push_back("TextureName\t" + m_SceneObjects[i].textureName);
        lines.push_back("TilingFactor\t" + std::to_string(m_SceneObjects[i].tilingFactor));
        std::string isSelected = m_SceneObjects[i].isSelected ? "1" : "0";
        lines.push_back("IsSelected\t" + isSelected);
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
        else if (tokens.size() >= 1 && tokens[0] == "EndObject") {
            sceneObject.transform = Math::CreateTransform(sceneObject.position, sceneObject.rotation, sceneObject.scale);
            sceneObject.AABB = new AABB(sceneObject.position, sceneObject.scale);
            sceneObject.pivot = new Pivot(sceneObject.position, sceneObject.scale);
            sceneObject.mesh = new Block(sceneObject.scale);
            m_SceneObjects.push_back(sceneObject);
            // printf("EndObject: New SceneObject added to m_SceneObjects...\n");
        }
    }
}

void SceneEditor::ResetScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneReset.lastTime < m_SceneReset.cooldown) return;
    m_SceneReset.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::ResetScene: Deleting %zu objects!\n", m_SceneObjects.size());

    for (auto& object : m_SceneObjects)
    {
        delete object.AABB;
        delete object.pivot;
        delete object.mesh;
    }

    m_SceneObjects.clear();
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
        m_PositionEdit =     &m_SceneObjects[m_SelectedIndex].position;
        m_RotationEdit =     &m_SceneObjects[m_SelectedIndex].rotation;
        m_ScaleEdit =        &m_SceneObjects[m_SelectedIndex].scale;
        m_ColorEdit =        &m_SceneObjects[m_SelectedIndex].color;
        m_UseTextureEdit =   &m_SceneObjects[m_SelectedIndex].useTexture;
        m_TextureNameEdit  = &m_SceneObjects[m_SelectedIndex].textureName;
        m_TilingFactorEdit = &m_SceneObjects[m_SelectedIndex].tilingFactor;
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
                    m_SceneObjects[m_SelectedIndex].textureName = items[n];
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
    ImGui::Separator();
    ImGui::SliderFloat("FOV", &m_FOV, 1.0f, 120.0f);

    struct Light
    {
        glm::vec3 color;
        float ambientIntensity;
        float diffuseIntensity;
    };

    struct DirectionalLight
    {
        Light base;
        glm::vec3 direction;
    };

    DirectionalLight directionalLight;

    directionalLight.base.color            = m_LightManager->directionalLight.GetColor();
    directionalLight.base.ambientIntensity = m_LightManager->directionalLight.GetAmbientIntensity();
    directionalLight.base.diffuseIntensity = m_LightManager->directionalLight.GetDiffuseIntensity();
    directionalLight.direction        = m_LightManager->directionalLight.GetDirection();

    ImGui::Separator();
    ImGui::Text("Directional Light");
    ImGui::ColorEdit3("DirLight Color", glm::value_ptr(directionalLight.base.color));
    ImGui::SliderFloat("DirLight Ambient Intensity", &directionalLight.base.ambientIntensity, 0.0f, 2.0f);
    ImGui::SliderFloat("DirLight Diffuse Intensity", &directionalLight.base.diffuseIntensity, 0.0f, 2.0f);
    ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(directionalLight.direction), -1.0f, 1.0f);

    m_LightManager->directionalLight.SetColor(directionalLight.base.color);
    m_LightManager->directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
    m_LightManager->directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);
    m_LightManager->directionalLight.SetDirection(directionalLight.direction);

    // Point Lights
    ImGui::Separator();
    ImGui::Text("Point Lights");
    ImGui::Separator();

    struct PointLight
    {
        Light base;
        glm::vec3 position;
        float constant;
        float linear;
        float exponent;
    };

    PointLight pointLights[4];
    char locBuff[100] = { '\0' };

    for (unsigned int pl = 0; pl < m_LightManager->pointLightCount; pl++)
    {
        pointLights[pl].base.color =            m_LightManager->pointLights[pl].GetColor();
        pointLights[pl].base.ambientIntensity = m_LightManager->pointLights[pl].GetAmbientIntensity();
        pointLights[pl].base.diffuseIntensity = m_LightManager->pointLights[pl].GetDiffuseIntensity();
        pointLights[pl].position =              m_LightManager->pointLights[pl].GetPosition();
        pointLights[pl].constant =              m_LightManager->pointLights[pl].GetConstant();
        pointLights[pl].linear =                m_LightManager->pointLights[pl].GetLinear();
        pointLights[pl].exponent =              m_LightManager->pointLights[pl].GetExponent();

        snprintf(locBuff, sizeof(locBuff), "Point Light %i", pl);
        ImGui::Text(locBuff);

        snprintf(locBuff, sizeof(locBuff), "PL %i Color", pl);
        ImGui::ColorEdit3(locBuff, glm::value_ptr(pointLights[pl].base.color));
        snprintf(locBuff, sizeof(locBuff), "PL %i Ambient Intensity", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].base.ambientIntensity, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Diffuse Intensity", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].base.diffuseIntensity, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Position", pl);
        ImGui::SliderFloat3(locBuff, glm::value_ptr(pointLights[pl].position), -20.0f, 20.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Constant", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].constant, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Linear", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].linear, -2.0f, 2.0f);
        snprintf(locBuff, sizeof(locBuff), "PL %i Exponent", pl);
        ImGui::SliderFloat(locBuff, &pointLights[pl].exponent, -2.0f, 2.0f);

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
    ImGui::Text("Spot Light 0");
    ImGui::Separator();
    ImGui::Text("Spot Light 1");
    ImGui::Separator();
    ImGui::Text("Spot Light 2");
    ImGui::Separator();
    ImGui::Text("Spot Light 3");

    ImGui::End();
}

void SceneEditor::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    Shader* shaderEditor = shaders["editor_object"];

    shaderEditor->Bind();
    shaderEditor->setMat4("projection", projectionMatrix);
    shaderEditor->setMat4("view", m_Camera->CalculateViewMatrix());

    // Set shader variables for Phong lighting model

    // Material
    shaderEditor->setFloat("material.specularIntensity", 1.0f);
    shaderEditor->setFloat("material.shininess", 64.0f);

    // Directional Light
    shaderEditor->setVec3("directionalLight.base.color", m_LightManager->directionalLight.GetColor());
    shaderEditor->setFloat("directionalLight.base.ambientIntensity", m_LightManager->directionalLight.GetAmbientIntensity());
    shaderEditor->setFloat("directionalLight.base.diffuseIntensity", m_LightManager->directionalLight.GetDiffuseIntensity());

    shaderEditor->setVec3("directionalLight.direction", m_LightManager->directionalLight.GetDirection());

    // Point Lights
    for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        char locBuff[100] = { '\0' };

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
        shaderEditor->setVec3(locBuff, m_LightManager->pointLights[i].GetColor());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
        shaderEditor->setFloat(locBuff, m_LightManager->pointLights[i].GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
        shaderEditor->setFloat(locBuff, m_LightManager->pointLights[i].GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
        shaderEditor->setVec3(locBuff, m_LightManager->pointLights[i].GetPosition());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
        shaderEditor->setFloat(locBuff, m_LightManager->pointLights[i].GetConstant());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
        shaderEditor->setFloat(locBuff, m_LightManager->pointLights[i].GetLinear());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
        shaderEditor->setFloat(locBuff, m_LightManager->pointLights[i].GetExponent());
    }

    shaderEditor->setInt("pointLightCount", MAX_POINT_LIGHTS);

    // Eye position / camera direction
    shaderEditor->setVec3("eyePosition", m_Camera->GetPosition());

    for (auto& object : m_SceneObjects)
    {
        object.transform = glm::mat4(1.0f);
        object.transform = glm::translate(object.transform, object.position);
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        // object.transform = glm::scale(object.transform, object.scale);
        shaderEditor->setMat4("model", object.transform);
        shaderEditor->setVec4("tintColor", object.color);
        shaderEditor->setBool("isSelected", object.isSelected);

        if (object.useTexture && object.textureName != "")
            textures[object.textureName]->Bind(0);
        else
            textures["plain"]->Bind(0);
        shaderEditor->setInt("albedoMap", 0);
        shaderEditor->setFloat("tilingFactor", object.tilingFactor);

        object.mesh->Render();

        object.AABB->Update(object.position, object.scale);
        object.pivot->Update(object.position, object.scale + 1.0f);
    }

    if (m_SelectedIndex < m_SceneObjects.size())
    {
        m_SceneObjects[m_SelectedIndex].AABB->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
        m_SceneObjects[m_SelectedIndex].pivot->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
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

    // Add Scene Object here
    SceneObject sceneObject = {
        glm::mat4(1.0f),
        defaultSpawnPosition,
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec4(1.0f),
        false,
        "plain",
        1.0f,
        true,
        new AABB(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
        new Pivot(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
        new Block(glm::vec3(1.0f, 1.5f, 1.0f)),
    };

    m_SceneObjects.push_back(sceneObject);
    m_SelectedIndex++;
}

void SceneEditor::CopySceneObject(SceneObject sceneObject)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectCopy.lastTime < m_ObjectCopy.cooldown) return;
    m_ObjectCopy.lastTime = m_CurrentTimestamp;

    Block* block = (Block*)sceneObject.mesh;

    sceneObject.isSelected = true;
    sceneObject.AABB = new AABB(sceneObject.position, sceneObject.scale);
    sceneObject.pivot = new Pivot(sceneObject.position, sceneObject.scale);
    sceneObject.mesh = new Block(block->GetScale());

    m_SceneObjects.push_back(sceneObject);
}

void SceneEditor::DeleteSceneObject()
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectDelete.lastTime < m_ObjectDelete.cooldown) return;
    m_ObjectDelete.lastTime = m_CurrentTimestamp;

    delete m_SceneObjects[m_SelectedIndex].AABB;
    delete m_SceneObjects[m_SelectedIndex].pivot;
    delete m_SceneObjects[m_SelectedIndex].mesh;

    if (m_SelectedIndex < m_SceneObjects.size())
        m_SceneObjects.erase(m_SceneObjects.begin() + m_SelectedIndex);

    if (m_SelectedIndex > 0) m_SelectedIndex--;
}

SceneEditor::~SceneEditor()
{
    SaveScene();

	CleanupGeometry();

    // delete m_PositionEdit;
    // delete m_RotationEdit;
    // delete m_ScaleEdit;
    // delete m_ColorEdit;
    // delete m_TextureNameEdit;
    // delete m_TilingFactorEdit;

    delete m_PivotScene;
    delete m_Grid;
    delete m_Raycast;

    ResetScene();
}