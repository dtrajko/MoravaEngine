#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneMarchingCubes.h"

#include "Camera/CameraControllerVoxelTerrain.h"
#include "Core/Math.h"
#include "Core/MousePicker.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "Mesh/Block.h"
#include "Terrain/NoiseSL.h"
#include "Terrain/TerrainMarchingCubes.h"


SceneMarchingCubes::SceneMarchingCubes()
{
    sceneSettings.cameraPosition = glm::vec3(10.0f, 24.0f, 0.0f);
    sceneSettings.cameraStartYaw = 0.0f;
    sceneSettings.cameraStartPitch = 20.0f;
    sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.waterHeight = 0.0f;
    sceneSettings.waterWaveSpeed = 0.05f;
    sceneSettings.enablePointLights  = true;
    sceneSettings.enableSpotLights   = true;
    sceneSettings.enableShadows      = false;
    sceneSettings.enableOmniShadows  = false;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles    = false;
    sceneSettings.enableSkybox       = false;
    sceneSettings.farPlane = 400.0f;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(0.5f, 0.5f, 0.5f);
    sceneSettings.directionalLight.base.ambientIntensity = 0.75f;
    sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;
    sceneSettings.lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 40.0f);

    // point lights
    sceneSettings.pointLights[0].base.enabled = false;
    sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[0].position = glm::vec3(-1.0f, 4.0f, 1.0f);
    sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[0].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[1].base.enabled = false;
    sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[1].position = glm::vec3(5.0f, 2.0f, 5.0f);
    sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[1].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[2].base.enabled = false;
    sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, -2.0f);
    sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[2].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[3].base.enabled = false;
    sceneSettings.pointLights[3].base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[3].position = glm::vec3(5.0f, 2.0f, -5.0f);
    sceneSettings.pointLights[3].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[3].base.diffuseIntensity = 1.0f;

    // spot lights
    sceneSettings.spotLights[0].base.base.enabled = false;
    sceneSettings.spotLights[0].base.base.color = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.position = glm::vec3(-5.0f, 2.0f, 0.0f);
    sceneSettings.spotLights[0].direction = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[0].edge = 0.5f;

    sceneSettings.spotLights[1].base.base.enabled = false;
    sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].base.position = glm::vec3(5.0f, 2.0f, 0.0f);
    sceneSettings.spotLights[1].direction = glm::vec3(-1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[1].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[1].edge = 0.5f;

    sceneSettings.spotLights[2].base.base.enabled = false;
    sceneSettings.spotLights[2].base.base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 2.0f, -5.0f);
    sceneSettings.spotLights[2].direction = glm::vec3(0.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[2].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[2].edge = 0.5f;

    sceneSettings.spotLights[3].base.base.enabled = false;
    sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 2.0f, 5.0f);
    sceneSettings.spotLights[3].direction = glm::vec3(0.0f, 0.0f, -1.0f);
    sceneSettings.spotLights[3].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[3].edge = 0.5f;

    ResourceManager::Init();

    /**** Configure Map Generator ***/
    m_MapGenConf.heightMapFilePath = "Textures/Noise/heightMap.png";
    m_MapGenConf.colorMapFilePath = "Textures/Noise/colorMap.png";
    m_MapGenConf.drawMode = MapGenerator::DrawMode::Mesh;
    m_MapGenConf.mapChunkSize = 32;
    // m_MapGenConf.mapWidth = 241;
    // m_MapGenConf.mapHeight = 241;
    m_MapGenConf.noiseScale = 25.0f;
    m_MapGenConf.octaves = 3;
    m_MapGenConf.persistance = 0.5f;
    m_MapGenConf.lacunarity = 2.0f;
    m_MapGenConf.seed = 123456;
    m_MapGenConf.offset = glm::vec2(0.0f, 0.0f);
    
    m_MapGenConf.autoUpdate = true;
    m_MapGenConf.regions = std::vector<MapGenerator::TerrainTypes>();
    
    m_HeightMapMultiplier = 12;
    m_HeightMapMultiplierPrev = m_HeightMapMultiplier;
    m_SeaLevel = 0.2f;
    m_SeaLevelPrev = m_SeaLevel;
    m_LevelOfDetail = 0;
    m_LevelOfDetailPrev = m_LevelOfDetail;

    m_VoxelsModified = false;

    m_DrawGizmos = true;
    m_RenderPlayer = true;
    m_RenderTerrainVoxels = false;
    m_RenderTerrainMarchingCubes = true;
    m_UnlockRotation = false;
    m_UnlockRotationPrev = m_UnlockRotation;

    SetCamera();
    SetLightManager();
    SetupTextureSlots();
    SetupTextures();
    SetupMeshes();

    m_UpdateCooldown = { 0.0f, 0.1f };
    m_DigCooldown = { 0.0f, 0.1f };
    m_RayIntersectCooldown = { 0.0f, 0.1f };
    m_RayCastCooldown = { 0.0f, 0.1f };
    m_OnClickCooldown = { 0.0f, 0.05f };

    /**** BEGIN Procedural Landmass Generation Terrain ****/
    m_IsRequiredMapUpdate = true;
    m_IsRequiredMapRebuild = true;

    NoiseSL::Init(m_MapGenConf.seed);
    m_TerrainMarchingCubes = new TerrainMarchingCubes(m_MapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, m_SeaLevel, m_LevelOfDetail, this);
    m_TotalVoxelNumber = (unsigned int)m_TerrainMarchingCubes->m_Voxels.size();

    m_TerrainEditMode = false;

    ResourceManager::LoadTexture("heightMap", m_MapGenConf.heightMapFilePath, GL_NEAREST, true);
    ResourceManager::LoadTexture("colorMap", m_MapGenConf.colorMapFilePath, GL_NEAREST, true);

    m_RenderInstanced = new RenderInstanced(m_TerrainMarchingCubes, ResourceManager::GetTexture("diffuse"), meshes["cube"]);

    /**** END Procedural Landmass Generation Terrain ****/

    Mesh* mesh = new Block();
    m_Player = new Player(glm::vec3(-40.0f, 20.0f, 0.0f), mesh, m_Camera);
    m_PlayerController = new PlayerController(m_Player);
    m_PlayerController->SetTerrain(m_TerrainMarchingCubes);
    m_PlayerController->SetGravity(0.0f);
    m_PlayerController->SetMoveSpeed(0.5f);
    m_PlayerController->SetMoveFastFactor(4.0f);

    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(300.0f, 300.0f, 300.0f));

    m_DigDistance = 1.6f;

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    MousePicker::Get()->SetTerrain(m_TerrainMarchingCubes);

    m_IntersectPosition = glm::ivec3(-1);

    m_CubeColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    m_DeleteVoxelCodeGLFW = GLFW_KEY_TAB;
}

void SceneMarchingCubes::SetCamera()
{
    m_Camera = new Camera(sceneSettings.cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f),
        sceneSettings.cameraStartYaw, sceneSettings.cameraStartPitch);

	m_CameraController = new CameraControllerVoxelTerrain(m_Camera, m_Player, m_AspectRatio, sceneSettings.cameraMoveSpeed, 0.1f, -0.7f);
}

void SceneMarchingCubes::SetupTextures()
{
    ResourceManager::LoadTexture("diffuse", "Textures/plain.png");
    ResourceManager::LoadTexture("normal",  "Textures/normal_map_default.png");
}

void SceneMarchingCubes::SetupTextureSlots()
{
    textureSlots.insert(std::make_pair("diffuse",    1));
    textureSlots.insert(std::make_pair("normal",     2));
    textureSlots.insert(std::make_pair("shadow",     3));
    textureSlots.insert(std::make_pair("omniShadow", 4));
    textureSlots.insert(std::make_pair("reflection", 5));
    textureSlots.insert(std::make_pair("refraction", 6));
    textureSlots.insert(std::make_pair("depth",      7));
    textureSlots.insert(std::make_pair("DuDv",       8));
}

void SceneMarchingCubes::SetupMeshes()
{
    Block* cube = new Block(glm::vec3(1.0f, 1.0f, 1.0f));
    meshes.insert(std::make_pair("cube", cube));
}

void SceneMarchingCubes::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    ImGui::Begin("Lights");
    {
        if (ImGui::CollapsingHeader("Directional Light"))
        {
            // Directional Light
            SDirectionalLight directionalLight;
            directionalLight.base.enabled = LightManager::directionalLight.GetEnabled();
            directionalLight.base.color = LightManager::directionalLight.GetColor();
            directionalLight.base.ambientIntensity = LightManager::directionalLight.GetAmbientIntensity();
            directionalLight.base.diffuseIntensity = LightManager::directionalLight.GetDiffuseIntensity();
            directionalLight.direction = LightManager::directionalLight.GetDirection();

            ImGui::Checkbox("DL Enabled", &directionalLight.base.enabled);
            ImGui::ColorEdit3("DL Color", glm::value_ptr(directionalLight.base.color));
            ImGui::SliderFloat3("DL Direction", glm::value_ptr(directionalLight.direction), -1.0f, 1.0f);
            ImGui::SliderFloat("DL Ambient Intensity", &directionalLight.base.ambientIntensity, 0.0f, 4.0f);
            ImGui::SliderFloat("DL Diffuse Intensity", &directionalLight.base.diffuseIntensity, 0.0f, 4.0f);

            LightManager::directionalLight.SetEnabled(directionalLight.base.enabled);
            LightManager::directionalLight.SetColor(directionalLight.base.color);
            LightManager::directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
            LightManager::directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);
            LightManager::directionalLight.SetDirection(directionalLight.direction);
        }

        if (ImGui::CollapsingHeader("Point Lights"))
        {
            ImGui::Indent();

            // Point Lights
            SPointLight pointLights[4];
            char locBuff[100] = { '\0' };
            for (unsigned int pl = 0; pl < LightManager::pointLightCount; pl++)
            {
                pointLights[pl].base.enabled = LightManager::pointLights[pl].GetEnabled();
                pointLights[pl].base.color = LightManager::pointLights[pl].GetColor();
                pointLights[pl].base.ambientIntensity = LightManager::pointLights[pl].GetAmbientIntensity();
                pointLights[pl].base.diffuseIntensity = LightManager::pointLights[pl].GetDiffuseIntensity();
                pointLights[pl].position = LightManager::pointLights[pl].GetPosition();
                pointLights[pl].constant = LightManager::pointLights[pl].GetConstant();
                pointLights[pl].linear = LightManager::pointLights[pl].GetLinear();
                pointLights[pl].exponent = LightManager::pointLights[pl].GetExponent();

                snprintf(locBuff, sizeof(locBuff), "Point Light %i", pl);
                if (ImGui::CollapsingHeader(locBuff))
                {
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
                }

                LightManager::pointLights[pl].SetEnabled(pointLights[pl].base.enabled);
                LightManager::pointLights[pl].SetColor(pointLights[pl].base.color);
                LightManager::pointLights[pl].SetAmbientIntensity(pointLights[pl].base.ambientIntensity);
                LightManager::pointLights[pl].SetDiffuseIntensity(pointLights[pl].base.diffuseIntensity);
                LightManager::pointLights[pl].SetPosition(pointLights[pl].position);
                LightManager::pointLights[pl].SetConstant(pointLights[pl].constant);
                LightManager::pointLights[pl].SetLinear(pointLights[pl].linear);
                LightManager::pointLights[pl].SetExponent(pointLights[pl].exponent);
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Spot Lights"))
        {
            ImGui::Indent();

            // Spot Lights
            SSpotLight spotLights[4];
            char locBuff[100] = { '\0' };
            for (unsigned int sl = 0; sl < LightManager::spotLightCount; sl++)
            {
                spotLights[sl].base.base.enabled = LightManager::spotLights[sl].GetBasePL()->GetEnabled();
                spotLights[sl].base.base.color = LightManager::spotLights[sl].GetBasePL()->GetColor();
                spotLights[sl].base.base.ambientIntensity = LightManager::spotLights[sl].GetBasePL()->GetAmbientIntensity();
                spotLights[sl].base.base.diffuseIntensity = LightManager::spotLights[sl].GetBasePL()->GetDiffuseIntensity();
                spotLights[sl].base.position = LightManager::spotLights[sl].GetBasePL()->GetPosition();
                spotLights[sl].base.constant = LightManager::spotLights[sl].GetBasePL()->GetConstant();
                spotLights[sl].base.linear = LightManager::spotLights[sl].GetBasePL()->GetLinear();
                spotLights[sl].base.exponent = LightManager::spotLights[sl].GetBasePL()->GetExponent();
                spotLights[sl].direction = LightManager::spotLights[sl].GetDirection();
                spotLights[sl].edge = LightManager::spotLights[sl].GetEdge();

                snprintf(locBuff, sizeof(locBuff), "Spot Light %i", sl);
                if (ImGui::CollapsingHeader(locBuff))
                {
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
                }

                LightManager::spotLights[sl].GetBasePL()->SetEnabled(spotLights[sl].base.base.enabled);
                LightManager::spotLights[sl].GetBasePL()->SetColor(spotLights[sl].base.base.color);
                LightManager::spotLights[sl].GetBasePL()->SetAmbientIntensity(spotLights[sl].base.base.ambientIntensity);
                LightManager::spotLights[sl].GetBasePL()->SetDiffuseIntensity(spotLights[sl].base.base.diffuseIntensity);
                LightManager::spotLights[sl].GetBasePL()->SetPosition(spotLights[sl].base.position);
                LightManager::spotLights[sl].GetBasePL()->SetConstant(spotLights[sl].base.constant);
                LightManager::spotLights[sl].GetBasePL()->SetLinear(spotLights[sl].base.linear);
                LightManager::spotLights[sl].GetBasePL()->SetExponent(spotLights[sl].base.exponent);
                LightManager::spotLights[sl].SetDirection(spotLights[sl].direction);
                LightManager::spotLights[sl].SetEdge(spotLights[sl].edge);
            }
            ImGui::Unindent();
        }
    }
    ImGui::End();

    ImGui::Begin("Debug");
    {
        if (ImGui::CollapsingHeader("Show Details"))
        {
            CameraControllerVoxelTerrain* cameraController = (CameraControllerVoxelTerrain*)m_CameraController;
            ImGui::SliderFloat3("Player Position", glm::value_ptr(cameraController->m_DebugPlayerPosition), -200.0f, 200.0f);
            ImGui::SliderFloat3("Camera Position", glm::value_ptr(cameraController->m_DebugCameraPosition), -200.0f, 200.0f);
            ImGui::SliderFloat3("Player Front", glm::value_ptr(cameraController->m_DebugPlayerFront), -200.0f, 200.0f);
            ImGui::SliderFloat3("Camera Front", glm::value_ptr(cameraController->m_DebugCameraFront), -200.0f, 200.0f);
            ImGui::SliderFloat("Camera Pitch", &cameraController->m_DebugCameraPitch, -1000.0f, 1000.0f);
            ImGui::SliderFloat("Camera Yaw", &cameraController->m_DebugCameraYaw, -1000.0f, 1000.0f);
            ImGui::SliderFloat("Camera Angle Around Player", &cameraController->m_DebugAngleAroundPlayer, -1000.0f, 1000.0f);
            ImGui::SliderFloat("Theta Horizontal", &cameraController->m_DebugTheta, -1000.0f, 1000.0f);
        }
    }
    ImGui::End();

    ImGui::Begin("Ray Casting");
    {
        if (ImGui::CollapsingHeader("Show Details"))
        {
            MousePicker* mp = MousePicker::Get();

            char buffer[50];

            sprintf(buffer, "Screen Mouse Coord: [ %i, %i ]", mp->m_ScreenMouseX, mp->m_ScreenMouseY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Viewport Mouse Coord: [ %i, %i ]", mp->m_Viewport.MouseX, mp->m_Viewport.MouseY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Normalized Coord: [ %.3f %.3f ]", mp->m_NormalizedCoords.x, mp->m_NormalizedCoords.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Clip Coord: [ %.3f %.3f ]", mp->m_ClipCoords.x, mp->m_ClipCoords.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Eye Coord: [ %.3f %.3f %.3f %.3f ]", mp->m_EyeCoords.x, mp->m_EyeCoords.y, mp->m_EyeCoords.z, mp->m_EyeCoords.w);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "World Ray: [ %.3f, %.3f %.3f ]", mp->m_WorldRay.x, mp->m_WorldRay.y, mp->m_WorldRay.z);
            ImGui::Text(buffer);
            ImGui::Separator();

            ImGui::SliderFloat3("Test Point", glm::value_ptr(mp->m_TestPoint), -200.0f, 200.0f);
            ImGui::SliderFloat3("Ray Start", glm::value_ptr(m_Raycast->m_LineStart), -200.0f, 200.0f);
            ImGui::SliderFloat3("Ray End", glm::value_ptr(m_Raycast->m_LineEnd), -200.0f, 200.0f);
            ImGui::ColorEdit4("Ray Color", glm::value_ptr(m_Raycast->m_Color));
            ImGui::Separator();
            ImGui::SliderFloat3("Intersection point", glm::value_ptr(mp->m_IntersectionPoint), -200.0f, 200.0f);
        }
    }
    ImGui::End();

    ImGui::Begin("Scene Settings");
    {
        ImGui::Checkbox("Draw Gizmos", &m_DrawGizmos);
        ImGui::Checkbox("Render Player", &m_RenderPlayer);
        ImGui::Checkbox("Render Voxel Terrain", &m_RenderTerrainVoxels);
        ImGui::Checkbox("Render Marching Cubes Terrain", &m_RenderTerrainMarchingCubes);
        ImGui::Checkbox("Terrain Edit Mode", &m_TerrainEditMode);
        ImGui::Checkbox("Unlock Rotation", &m_UnlockRotation);
        ImGui::Checkbox("Enable Shadows", &sceneSettings.enableShadows);
        ImGui::ColorEdit4("Cube Color", glm::value_ptr(m_CubeColor));
    }
    ImGui::End();

    ImGui::Begin("Help");
    {
        ImGui::Text("* Add voxel - Mouse Left Button");
        ImGui::Text("* Delete Voxel - Mouse Left Button + [TAB]");
        ImGui::Text("* Rotate Camera around Player: Mouse Left Button + [C]");
        ImGui::Text("* Dig - [F]");
    }
    ImGui::End();

    ImGui::Begin("Terrain Parameters");
    {
        if (ImGui::CollapsingHeader("Show Details"))
        {
            std::string totalVoxelNumber = "Total number of voxels: " + std::to_string(m_TotalVoxelNumber);
            ImGui::Text(totalVoxelNumber.c_str());
        }
    }
    ImGui::End();

    ImGui::Begin("Map Generator");
    {
        if (ImGui::CollapsingHeader("Show Details"))
        {
            // Begin DrawMode ImGui drop-down list
            static const char* items[]{ "HeightMap", "ColorMap", "Mesh" };
            static int selectedItem = (int)m_MapGenConf.drawMode;
            ImGui::Combo("Draw Mode", &selectedItem, items, IM_ARRAYSIZE(items));
            m_MapGenConf.drawMode = (MapGenerator::DrawMode)selectedItem;
            // End DrawMode ImGui drop-down list

            // ImGui::SliderInt("Map Width", &m_MapGenConf.mapWidth,   1, 512);
            // ImGui::SliderInt("Map Height", &m_MapGenConf.mapHeight, 1, 512);
            ImGui::SliderInt("Map Chunk Size", &m_MapGenConf.mapChunkSize, 0, 256);
            ImGui::SliderInt("Level Of Detail", &m_LevelOfDetail, 0, 6);
            ImGui::SliderFloat("Noise Scale", &m_MapGenConf.noiseScale, 1.0f, 100.0f);
            ImGui::SliderInt("Octaves", &m_MapGenConf.octaves, 1, 10);
            ImGui::SliderFloat("Persistance", &m_MapGenConf.persistance, 0.0f, 1.0f);
            ImGui::SliderFloat("Lacunarity", &m_MapGenConf.lacunarity, 1.0f, 5.0f);
            ImGui::SliderInt("Seed", &m_MapGenConf.seed, 0, 100000);
            ImGui::SliderFloat2("Offset", glm::value_ptr(m_MapGenConf.offset), -1.0f, 1.0f);
            ImGui::Checkbox("Auto Update", &m_MapGenConf.autoUpdate);

            ImGui::SliderInt("Height Map Multiplier", &m_HeightMapMultiplier, 0, 64);
            ImGui::SliderFloat("Sea Level", &m_SeaLevel, 0.0f, 1.0f);
        }
    }
    ImGui::End();

    ImGui::Begin("Profiler");
    {
        float realFPS = Timer::Get()->GetRealFPS();
        std::string sRealFPS = "Real FPS: " + std::to_string(realFPS);

        float deltaTimeMS = Timer::Get()->GetDeltaTime() * 1000.0f;
        std::string sDeltaTimeMS = "Delta Time: " + std::to_string(deltaTimeMS) + " ms";

        if (ImGui::CollapsingHeader("Timer"))
        {
            ImGui::Text(sRealFPS.c_str());
            ImGui::Text(sDeltaTimeMS.c_str());
        }

        if (ImGui::CollapsingHeader("Active Render Passes"))
        {
            ImGui::Indent();
            for (auto& renderPassName : m_ActiveRenderPasses)
                ImGui::Text(renderPassName.c_str());
            ImGui::Unindent();
        }

        m_ActiveRenderPasses.clear();

        // print profiler results
        if (ImGui::CollapsingHeader("Profiler results:"))
        {
            // profiler results
            for (auto& profilerResult : m_ProfilerResults)
            {
                char label[100];
                strcpy(label, "%.2fms ");
                strcat(label, profilerResult.first.c_str());
                ImGui::Text(label, profilerResult.second);
            }
            m_ProfilerResults.clear();
        }
    }
    ImGui::End();

    ImGui::Begin("Framebuffers");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            ImVec2 imageSize(128.0f, 128.0f);

            ImGui::Text("Shadow Map");
            ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);
        }
    }
    ImGui::End();
}

void SceneMarchingCubes::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);

    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    Dig(mainWindow->getKeys(), timestep);
    CastRay(mainWindow->getKeys(), mainWindow->getMouseButtons(), timestep);
    OnClick(mainWindow->getKeys(), mainWindow->getMouseButtons(), timestep);

    {
        Profiler profiler("SMC::UpdateCooldown");
        UpdateCooldown(timestep, mainWindow);
        GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }

    m_PlayerController->KeyControl(mainWindow->getKeys(), timestep);
    m_PlayerController->MouseControl(mainWindow->getMouseButtons(), mainWindow->getXChange(), mainWindow->getYChange());
    m_PlayerController->MouseScrollControl(mainWindow->getKeys(), timestep, mainWindow->getXMouseScrollOffset(), mainWindow->getYMouseScrollOffset());
    m_PlayerController->SetUnlockRotation(m_UnlockRotation);
    m_Player->Update();
    // m_CameraController->Update(); // called from main.cpp
    m_CameraController->SetUnlockRotation(m_UnlockRotation);

    m_DeleteMode = mainWindow->getKeys()[m_DeleteVoxelCodeGLFW];

    m_RenderInstanced->Update();
    m_RenderInstanced->SetIntersectPosition(m_IntersectPosition);
    m_RenderInstanced->SetDeleteMode(&m_DeleteMode);

    if (m_UnlockRotation != m_UnlockRotationPrev) {
        if (m_UnlockRotation) {
            mainWindow->SetCursorDisabled();
        }
        else {
            mainWindow->SetCursorNormal();
        }
        m_UnlockRotationPrev = m_UnlockRotation;
    }
}

void SceneMarchingCubes::CheckMapRebuildRequirements()
{
    m_IsRequiredMapUpdate = false;
    m_IsRequiredMapRebuild = false;

    if (m_MapGenConf != m_MapGenConfPrev) {
        m_IsRequiredMapUpdate = true;
        m_IsRequiredMapRebuild = true;
        m_MapGenConfPrev = m_MapGenConf;
    }

    if (m_HeightMapMultiplier != m_HeightMapMultiplierPrev ||
        m_SeaLevel != m_SeaLevelPrev ||
        m_LevelOfDetail != m_LevelOfDetailPrev) {
        m_IsRequiredMapUpdate = true;
        m_IsRequiredMapRebuild = false;
        m_HeightMapMultiplierPrev = m_HeightMapMultiplier;
        m_SeaLevelPrev = m_SeaLevel;
        m_LevelOfDetailPrev = m_LevelOfDetail;
    }
}

void SceneMarchingCubes::UpdateCooldown(float timestep, Window* mainWindow)
{
    // Cooldown
    if (timestep - m_UpdateCooldown.lastTime < m_UpdateCooldown.cooldown) return;
    m_UpdateCooldown.lastTime = timestep;

    /**** BEGIN UpdateCooldown Procedural Landmass Generation Terrain ****/

    if (m_VoxelsModified) {
        {
            Profiler profiler("SMC: m_RenderInstanced->CreateVertexData");
            m_RenderInstanced->CreateVertexData();
            GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));

        }
        {
            Profiler profiler("SMC: m_TerrainMarchingCubes->MarchingCubes");
            m_TerrainMarchingCubes->MarchingCubes();
            GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
        }
        m_VoxelsModified = false;
    }

    CheckMapRebuildRequirements();

    if (!m_IsRequiredMapUpdate) return;

    // printf("SceneMarchingCubes::UpdateCooldown m_VoxelsModified = %i\n", m_VoxelsModified);

    {
        Profiler profiler("SMC::TerrainMarchingCubes::Update");
        m_TerrainMarchingCubes->Update(m_MapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, m_SeaLevel, m_LevelOfDetail);
        GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }

    ResourceManager::LoadTexture("heightMap", m_MapGenConf.heightMapFilePath, GL_NEAREST, true);
    ResourceManager::LoadTexture("colorMap", m_MapGenConf.colorMapFilePath, GL_NEAREST, true);

    /**** END UpdateCooldown Procedural Landmass Generation Terrain ****/

    m_TotalVoxelNumber = (unsigned int)m_TerrainMarchingCubes->m_Voxels.size();
}

void SceneMarchingCubes::CastRay(bool* keys, bool* buttons, float timestep)
{
    // Cooldown
    if (timestep - m_RayCastCooldown.lastTime < m_RayCastCooldown.cooldown) return;
    m_RayCastCooldown.lastTime = timestep;

    if (true || keys[GLFW_KEY_C] || buttons[GLFW_MOUSE_BUTTON_1])
    {
        m_IntersectPositionVector = GetRayIntersectPositions(timestep, m_Camera);
        if (m_IntersectPositionVector.size() > 0) {
            m_RenderInstanced->CreateVertexData();
        }
    }
}

std::vector<glm::vec3> SceneMarchingCubes::GetRayIntersectPositions(float timestep, Camera* camera)
{
    std::vector<glm::vec3> rayIntersectPositions = std::vector<glm::vec3>();

    // Cooldown
    if (timestep - m_RayIntersectCooldown.lastTime < m_RayIntersectCooldown.cooldown) return rayIntersectPositions;
    m_RayIntersectCooldown.lastTime = timestep;

    constexpr float maxFloatValue = std::numeric_limits<float>::max();
    float minimalDistance = maxFloatValue;
    float distance;
    glm::ivec3 position;

    for (auto it = m_TerrainMarchingCubes->m_Voxels.begin(); it != m_TerrainMarchingCubes->m_Voxels.end(); it++) {
        position = it->second->position;
        bool isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            (glm::vec3)position - glm::vec3(0.5f, 0.5f, 0.5f), (glm::vec3)position + glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(0.0f));
        if (isSelected) {
            rayIntersectPositions.push_back(position);
        
            // find position nearest to camera
            distance = glm::distance((glm::vec3)position, camera->GetPosition());
            if (distance < minimalDistance) {
                minimalDistance = distance;
                m_IntersectPosition = position;
            }
        }
    }

    return rayIntersectPositions;
}

bool SceneMarchingCubes::IsRayIntersectPosition(glm::vec3 position)
{
    for (auto rayIntersectPosition : m_IntersectPositionVector) {
        if (position == rayIntersectPosition) {
            return true;
        }
    }
    return false;
}

void SceneMarchingCubes::OnClick(bool* keys, bool* buttons, float timestep)
{
    // Cooldown
    if (timestep - m_OnClickCooldown.lastTime < m_OnClickCooldown.cooldown) return;
    m_OnClickCooldown.lastTime = timestep;

    if (!m_TerrainEditMode) return;

    if (buttons[GLFW_MOUSE_BUTTON_1])
    {
        // Delete current voxel
        if (keys[m_DeleteVoxelCodeGLFW]) {
            Profiler profiler("SMC::DeleteVoxel");
            DeleteVoxel();
            GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
        }
        else {
            Profiler profiler("SMC::AddVoxel");
            AddVoxel();
            GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
        }
    }
}

void SceneMarchingCubes::AddVoxel()
{
    // Add new voxel
    glm::vec3 addPositionFloat = (glm::vec3)m_IntersectPosition - m_Camera->GetFront();
    glm::ivec3 addPositionInt = glm::ivec3(std::round(addPositionFloat.x), std::round(addPositionFloat.y), std::round(addPositionFloat.z));

    for (int x = addPositionInt.x - 1; x < addPositionInt.x + 1; x++) {
        for (int y = addPositionInt.y - 1; y < addPositionInt.y + 1; y++) {
            for (int z = addPositionInt.z - 1; z < addPositionInt.z + 1; z++) {
                glm::ivec3 subPositionInt = glm::ivec3(x, y, z);
                if (IsPositionVacant(subPositionInt)) {
                    TerrainVoxel::Voxel* voxel = new TerrainVoxel::Voxel();
                    voxel->position = subPositionInt;
                    voxel->color = glm::vec4(m_CubeColor);
                    m_TerrainMarchingCubes->m_Voxels.insert(std::make_pair(m_TerrainMarchingCubes->GetVoxelMapKey(voxel->position), voxel));
                    m_IntersectPosition = voxel->position;
                    m_VoxelsModified = true;
                    Log::GetLogger()->info("New voxel at position [ {0} {1} {2} ] added!", subPositionInt.x, subPositionInt.y, subPositionInt.z);
                }
                else {
                    Log::GetLogger()->warn("Voxel at position [ {0} {1} {2} ] already exists!", subPositionInt.x, subPositionInt.y, subPositionInt.z);
                }
            }
        }
    }
}

void SceneMarchingCubes::DeleteVoxel()
{
    if (m_IntersectPosition == glm::ivec3(-1, -1, -1)) return;

    auto voxelEntry = m_TerrainMarchingCubes->m_Voxels.find(m_TerrainMarchingCubes->GetVoxelMapKey(m_IntersectPosition));
    if (voxelEntry != m_TerrainMarchingCubes->m_Voxels.end())
    {
        glm::ivec3 deletePosition = voxelEntry->second->position;
        for (int x = deletePosition.x - 1; x < deletePosition.x + 1; x++) {
            for (int y = deletePosition.y - 1; y < deletePosition.y + 1; y++) {
                for (int z = deletePosition.z - 1; z < deletePosition.z + 1; z++) {
                    bool voxelDeleted = m_TerrainMarchingCubes->DeleteVoxel(glm::ivec3(x, y, z));
                    m_VoxelsModified = true;
                    m_IntersectPosition = glm::ivec3(-1, -1, -1);
                    Log::GetLogger()->info("Voxel at position [ {0} {1} {2} ] deleted!", deletePosition.x, deletePosition.y, deletePosition.z);
                }
            }
        }
    }
}

bool SceneMarchingCubes::IsPositionVacant(glm::ivec3 queryPosition)
{
    for (auto voxel : m_TerrainMarchingCubes->m_Voxels) {
        if (voxel.second->position == queryPosition)
            return false;
    }
    return true;
}

void SceneMarchingCubes::Dig(bool* keys, float timestep)
{
    // Cooldown
    if (timestep - m_DigCooldown.lastTime < m_DigCooldown.cooldown) return;
    m_DigCooldown.lastTime = timestep;

    if (keys[GLFW_KEY_F]) {
        bool vectorModified = false;

        for (auto it = m_TerrainMarchingCubes->m_Voxels.begin(); it != m_TerrainMarchingCubes->m_Voxels.end(); ) {
            if (glm::distance(m_Player->GetPosition(), (glm::vec3)it->second->position) < m_DigDistance)
            {
                it = m_TerrainMarchingCubes->m_Voxels.erase(it++);
                vectorModified = true;
            }
            else {
                ++it;
            }
        }
        if (vectorModified) {
            m_RenderInstanced->CreateVertexData();
        }
    }
}

void SceneMarchingCubes::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    m_ActiveRenderPasses.push_back(passType); // for displaying all render passes in ImGui

    Shader* shaderMain = shaders["main"];
    Shader* shaderShadowMap = shaders["shadow_map"];
    Shader* shaderOmniShadow = shaders["omniShadow"];
    Shader* shaderRenderInstanced = shaders["render_instanced"];
    Shader* shaderBasic = shaders["basic"];
    Shader* shaderMarchingCubes = shaders["marching_cubes"];

    RendererBasic::EnableTransparency();

    ResourceManager::GetTexture("diffuse")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("normal")->Bind(textureSlots["normal"]);

    if (sceneSettings.enableShadows && LightManager::directionalLight.GetShadowMap() != nullptr)
        LightManager::directionalLight.GetShadowMap()->ReadTexture(GetTextureSlots()["shadow"]);

    m_RenderInstanced->m_Texture->Bind(0);

    if (m_DrawGizmos) {
        if (passType == "main") {
            shaderBasic->Bind();
            shaderBasic->setMat4("model", glm::mat4(1.0f));
            m_PivotScene->Draw(shaderBasic, projectionMatrix, m_Camera->GetViewMatrix());
        }
    }

    if (m_RenderPlayer)
    {
        /**** BEGIN render Player ****/
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_Player->GetPosition());
        model = glm::rotate(model, glm::radians(m_Player->GetRotation().x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_Player->GetRotation().y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_Player->GetRotation().z), glm::vec3(0.0f, 0.0f, 1.0f));

        if (passType == "shadow_dir") {
            shaderShadowMap->Bind();
            shaderShadowMap->setMat4("model", model);
            shaderShadowMap->setMat4("u_DirLightTransform", LightManager::directionalLight.CalculateLightTransform());
            shaderShadowMap->setBool("u_Animated", false);
        }
        else {
            shaderMain->Bind();
            shaderMain->setMat4("projection", projectionMatrix);
            shaderMain->setMat4("view", m_Camera->GetViewMatrix());
            shaderMain->setMat4("model", model);
            shaderMain->setVec4("tintColor", m_Player->GetColor());
            shaderMain->setInt("albedoMap", 0);
            shaderMain->setInt("shadowMap", GetTextureSlots()["shadow"]);
        }
        m_Player->Render();
        /**** END render Player ****/
    }

    if (m_RenderTerrainMarchingCubes)
    {
        /**** BEGIN Render Marching Cubes ****/
        if (passType == "shadow_dir")
        {
            shaderShadowMap->Bind();
            shaderShadowMap->setMat4("model", glm::mat4(1.0f));
            shaderShadowMap->setMat4("u_DirLightTransform", LightManager::directionalLight.CalculateLightTransform());
            shaderShadowMap->setBool("u_Animated", false);
        }
        else
        {
            shaderMarchingCubes->Bind();
            shaderMarchingCubes->setMat4("projection", projectionMatrix);
            shaderMarchingCubes->setMat4("view", m_Camera->GetViewMatrix());
            shaderMarchingCubes->setMat4("model", glm::mat4(1.0f));
            shaderMarchingCubes->setVec4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            shaderMarchingCubes->setInt("albedoMap", 0);
            if (sceneSettings.enableShadows) {
                shaderMarchingCubes->setInt("shadowMap", GetTextureSlots()["shadow"]);
            }
        }
        m_TerrainMarchingCubes->Render();
        /**** END Render Marching Cubes ****/
    }

    if (m_RenderTerrainVoxels) {
        /**** BEGIN Render Procedural Landmass Generation Terrain ****/
        shaderRenderInstanced->Bind();
        shaderRenderInstanced->setMat4("projection", projectionMatrix);
        shaderRenderInstanced->setMat4("view", m_Camera->GetViewMatrix());
        shaderRenderInstanced->setVec4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        shaderRenderInstanced->setInt("albedoMap", 0);
        m_RenderInstanced->Render();
        /**** END Render Procedural Landmass Generation Terrain ****/
    }
}

void SceneMarchingCubes::Release()
{
    // if (m_TerrainSL) delete m_TerrainSL;
    // if (m_RenderInstanced) delete m_RenderInstanced;
}

SceneMarchingCubes::~SceneMarchingCubes()
{
    Release();

    delete m_Raycast;
    delete m_Player;

    for (auto mesh : meshes)
        delete &mesh;
}
