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

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_SelectedIndex = 0;

    m_SceneObjects.resize(2);

    m_SceneObjects[0].position = glm::vec3(2.0f, 0.5f, 2.0f);
    m_SceneObjects[1].position = glm::vec3(-2.0f, 0.5f, -2.0f);

    m_SceneObjects[0].rotation = glm::vec3(0.0f);
    m_SceneObjects[1].rotation = glm::vec3(0.0f);

    m_SceneObjects[0].scale = glm::vec3(1.0f);
    m_SceneObjects[1].scale = glm::vec3(1.0f);

    m_SceneObjects[0].color = glm::vec4(0.8f, 0.4f, 0.0f, 0.8f);
    m_SceneObjects[1].color = glm::vec4(0.4f, 0.4f, 0.8f, 0.8f);

    m_SceneObjects[0].useTexture = false;
    m_SceneObjects[1].useTexture = false;

    m_SceneObjects[0].textureName = "texture_checker";
    m_SceneObjects[0].textureName = "texture_checker";

    m_SceneObjects[0].tilingFactor = 1.0f;
    m_SceneObjects[1].tilingFactor = 1.0f;

    m_SceneObjects[0].isSelected = false;
    m_SceneObjects[1].isSelected = false;

    m_SceneObjects[0].AABB = new AABB(m_SceneObjects[0].position, m_SceneObjects[0].scale);
    m_SceneObjects[1].AABB = new AABB(m_SceneObjects[1].position, m_SceneObjects[1].scale);

    m_SceneObjects[0].pivot = new Pivot(m_SceneObjects[0].position, m_SceneObjects[0].scale + 1.0f);
    m_SceneObjects[1].pivot = new Pivot(m_SceneObjects[1].position, m_SceneObjects[1].scale + 1.0f);

    m_SceneObjects[0].mesh = new Block(glm::vec3(1.0f, 1.0f, 1.0f));
    m_SceneObjects[1].mesh = new Block(glm::vec3(1.0f, 1.0f, 1.0f));

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    m_Grid = new Grid(10);

    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(40.0f, 40.0f, 40.0f));
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
    textures.insert(std::make_pair("grass_albedo", new Texture("Textures/PBR/grass/albedo.png")));
    textures.insert(std::make_pair("wall_albedo", new Texture("Textures/PBR/wall/albedo.png")));
    textures.insert(std::make_pair("plastic_albedo", new Texture("Textures/PBR/plastic/albedo.png")));
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

    for (int i = 0; i < m_SceneObjects.size(); i++)
    {
        printf("Scene Object %i\n", i);
        printf("Position %.2ff %.2ff %.2ff\n", m_SceneObjects[i].position.x, m_SceneObjects[i].position.y, m_SceneObjects[i].position.z);
        printf("Rotation %.2ff %.2ff %.2ff\n", m_SceneObjects[i].rotation.x, m_SceneObjects[i].rotation.y, m_SceneObjects[i].rotation.z);
        printf("Scale %.2ff %.2ff %.2ff\n", m_SceneObjects[i].scale.x, m_SceneObjects[i].scale.y, m_SceneObjects[i].scale.z);
        printf("Color %.2ff %.2ff %.2ff %.2ff\n", m_SceneObjects[i].color.r, m_SceneObjects[i].color.g, m_SceneObjects[i].color.b, m_SceneObjects[i].color.a);
        printf("UseTexture %i\n", m_SceneObjects[i].useTexture ? 1 : 0);
        printf("TextureName %s\n", m_SceneObjects[i].textureName.c_str());
        printf("TilingFactor %.2ff\n", m_SceneObjects[i].tilingFactor);
        printf("IsSelected %i\n", m_SelectedIndex == i ? 1 : 0);
    }
}

void SceneEditor::LoadScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneLoad.lastTime < m_SceneLoad.cooldown) return;
    m_SceneLoad.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::LoadScene: Loading objects...\n");

    const char* sceneFilename = "Scenes/scene_01.scene";

    std::string sceneFileContent = Shader::ReadFile(sceneFilename);

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
            printf("ObjectID=%i\n", objectId);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Position") {
            sceneObject.position = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            printf("Position %.2ff %.2ff %.2ff\n", sceneObject.position.x, sceneObject.position.y, sceneObject.position.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Rotation") {
            sceneObject.rotation = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            printf("Rotation %.2ff %.2ff %.2ff\n", sceneObject.rotation.x, sceneObject.rotation.y, sceneObject.rotation.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Scale") {
            sceneObject.scale = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            printf("Scale %.2ff %.2ff %.2ff\n", sceneObject.scale.x, sceneObject.scale.y, sceneObject.scale.z);
        }
        else if (tokens.size() >= 5 && tokens[0] == "Color") {
            sceneObject.color = glm::vec4(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[4]));
            printf("Color %.2ff %.2ff %.2ff %.2ff\n", sceneObject.color.r, sceneObject.color.g, sceneObject.color.b, sceneObject.color.a);
        }
        else if (tokens.size() >= 2 && tokens[0] == "UseTexture") {
            sceneObject.useTexture = std::stoi(tokens[1]) == 1 ? true : false;
            printf("UseTexture %d\n", sceneObject.useTexture);
        }
        else if (tokens.size() >= 2 && tokens[0] == "TextureName") {
            sceneObject.textureName = tokens[1];
            printf("UseTexture %s\n", sceneObject.textureName.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "TilingFactor") {
            sceneObject.tilingFactor = std::stof(tokens[1]);
            printf("TilingFactor %.2f\n", sceneObject.tilingFactor);
        }
        else if (tokens.size() >= 2 && tokens[0] == "IsSelected") {
            sceneObject.isSelected = std::stoi(tokens[1]) == 1 ? true : false;
            if (sceneObject.isSelected) m_SelectedIndex = (unsigned int)m_SceneObjects.size();
            printf("IsSelected %d\n", sceneObject.isSelected);
        }
        else if (tokens.size() >= 1 && tokens[0] == "EndObject") {
            sceneObject.transform = Math::CreateTransform(sceneObject.position, sceneObject.rotation, sceneObject.scale);
            sceneObject.AABB = new AABB(sceneObject.position, sceneObject.scale);
            sceneObject.pivot = new Pivot(sceneObject.position, sceneObject.scale);
            sceneObject.mesh = new Block(sceneObject.scale);
            m_SceneObjects.push_back(sceneObject);
            printf("EndObject: New SceneObject added to m_SceneObjects...\n");
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

    if (m_SelectedIndex < m_SceneObjects.size())
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

    ImGui::End();
}

void SceneEditor::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    shaders["editor_object"]->Bind();
    shaders["editor_object"]->setMat4("projection", projectionMatrix);
    shaders["editor_object"]->setMat4("view", m_Camera->CalculateViewMatrix());

    for (auto& object : m_SceneObjects)
    {
        object.transform = glm::mat4(1.0f);
        object.transform = glm::translate(object.transform, object.position);
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        object.transform = glm::rotate(object.transform, glm::radians(object.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        // object.transform = glm::scale(object.transform, object.scale);
        shaders["editor_object"]->setMat4("model", object.transform);
        shaders["editor_object"]->setVec4("tintColor", object.color);
        shaders["editor_object"]->setBool("isSelected", object.isSelected);

        if (object.useTexture && object.textureName != "")
            textures[object.textureName]->Bind(0);
        else
            textures["plain"]->Bind(0);
        shaders["editor_object"]->setInt("albedoMap", 0);
        shaders["editor_object"]->setFloat("tilingFactor", object.tilingFactor);

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
	CleanupGeometry();

    delete m_Grid;
    delete m_Raycast;

    ResetScene();
}
