#include "Scene/SceneSponza.h"

#include "Mesh/MeshData.h"
#include "Mesh/Tile2D.h"
#include "Renderer/Renderer.h"


SceneSponza::SceneSponza()
{
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
    sceneSettings.enableShadows      = false;
    sceneSettings.enableOmniShadows  = false;
    sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(-4.0f, 10.0f, -0.5f);
	sceneSettings.cameraStartYaw = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.2f;
	sceneSettings.directionalLight.base.diffuseIntensity = 1.0f;
	sceneSettings.directionalLight.direction = glm::vec3(-1.0f, -89.0f, -1.0f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-32.0f, 32.0f, -32.0f, 32.0f, -32.0f, 32.0f);
    sceneSettings.pointLights[0].base.enabled = false;
    sceneSettings.pointLights[1].base.enabled = false;
    sceneSettings.pointLights[2].base.enabled = false;
    sceneSettings.pointLights[3].base.enabled = false;
    sceneSettings.spotLights[0].base.base.enabled = false;
    sceneSettings.spotLights[1].base.base.enabled = false;
    sceneSettings.spotLights[2].base.base.enabled = false;
    sceneSettings.spotLights[3].base.base.enabled = false;
    sceneSettings.shadowMapWidth = 1024;
	sceneSettings.shadowMapHeight = 1024;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 0.2f;
	sceneSettings.waterWaveSpeed = 0.02f;

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneSponza::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	m_Skybox = std::make_shared<Skybox>(skyboxFaces);
}

void SceneSponza::SetupTextures()
{
}

void SceneSponza::SetupMeshes()
{
	Mesh* quad = new Mesh();
	quad->Create(&MeshData::quadVertices[0], &MeshData::quadIndices[0], MeshData::quadVertexCount, MeshData::quadIndexCount);
	meshes.insert(std::make_pair("quad", quad));

	float vertices[] = { -1, -1, -1, 1, 1, -1, 1, -1, -1, 1, 1, 1 };
	unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };
	Tile2D* m_Tile2D = new Tile2D();
	m_Tile2D->Create(&vertices[0], &indices[0], 12, 6);
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void SceneSponza::SetupModels()
{
	Model* sponza = new Model("Models/sponza.obj", "Textures");
    models.insert(std::make_pair("sponza", sponza));
}

void SceneSponza::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);
}

void SceneSponza::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	if (m_ShowWindowSceneSettings)
	{
		ImGui::Begin("Scene Settings", &m_ShowWindowSceneSettings);
		{
			ImGui::Checkbox("enableShadows", &sceneSettings.enableShadows);
			ImGui::Checkbox("enableOmniShadows", &sceneSettings.enableOmniShadows);
			ImGui::Checkbox("enablePointLights", &sceneSettings.enablePointLights);
			ImGui::Checkbox("enableSpotLights", &sceneSettings.enableSpotLights);
			ImGui::Checkbox("enableWaterEffects", &sceneSettings.enableWaterEffects);

			ImGui::Separator();

			ImGui::SliderFloat("Water Level", &sceneSettings.waterHeight, -4.0f, 20.0f);
			m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);

			if (ImGui::SliderFloat("Water Wave Speed", &sceneSettings.waterWaveSpeed, -1.0f, 1.0f))
			{
				m_WaterManager->SetWaveSpeed(sceneSettings.waterWaveSpeed);
			}

			glm::vec4 waterColor = m_WaterManager->GetWaterColor();
			if (ImGui::ColorEdit4("Water Color", (float*)&waterColor))
			{
				m_WaterManager->SetWaterColor(waterColor);
			}
		}
		ImGui::End();
	}

	{
		// Directional Light
		SDirectionalLight directionalLight;
		directionalLight.base.enabled = LightManager::directionalLight.GetEnabled();
		directionalLight.base.color = LightManager::directionalLight.GetColor();
		directionalLight.base.ambientIntensity = LightManager::directionalLight.GetAmbientIntensity();
		directionalLight.base.diffuseIntensity = LightManager::directionalLight.GetDiffuseIntensity();
		directionalLight.direction = LightManager::directionalLight.GetDirection();

		// Point Lights
		SPointLight pointLights[4];
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
		}

		// Spot Lights
		SSpotLight spotLights[4];
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
		}

		if (m_ShowWindowLights)
		{
			ImGui::Begin("Lights", &m_ShowWindowLights);
			{
				if (ImGui::CollapsingHeader("Directional Light"))
				{
					ImGui::Checkbox("DL Enabled", &directionalLight.base.enabled);
					ImGui::ColorEdit3("DL Color", glm::value_ptr(directionalLight.base.color));
					ImGui::SliderFloat3("DL Direction", glm::value_ptr(directionalLight.direction), -100.0f, 100.0f);
					ImGui::SliderFloat("DL Ambient Intensity", &directionalLight.base.ambientIntensity, 0.0f, 4.0f);
					ImGui::SliderFloat("DL Diffuse Intensity", &directionalLight.base.diffuseIntensity, 0.0f, 4.0f);
				}

				if (ImGui::CollapsingHeader("Point Lights"))
				{
					ImGui::Indent();
					char locBuff[100] = { '\0' };
					for (unsigned int pl = 0; pl < LightManager::pointLightCount; pl++)
					{
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
					}
					ImGui::Unindent();
				}

				if (ImGui::CollapsingHeader("Spot Lights"))
				{
					ImGui::Indent();
					char locBuff[100] = { '\0' };
					for (unsigned int sl = 0; sl < LightManager::spotLightCount; sl++)
					{
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
					}

					ImGui::Unindent();
				}
			}
			ImGui::End();
		}

		LightManager::directionalLight.SetEnabled(directionalLight.base.enabled);
		LightManager::directionalLight.SetColor(directionalLight.base.color);
		LightManager::directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
		LightManager::directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);
		LightManager::directionalLight.SetDirection(directionalLight.direction);

		for (unsigned int pl = 0; pl < LightManager::pointLightCount; pl++)
		{
			LightManager::pointLights[pl].SetEnabled(pointLights[pl].base.enabled);
			LightManager::pointLights[pl].SetColor(pointLights[pl].base.color);
			LightManager::pointLights[pl].SetAmbientIntensity(pointLights[pl].base.ambientIntensity);
			LightManager::pointLights[pl].SetDiffuseIntensity(pointLights[pl].base.diffuseIntensity);
			LightManager::pointLights[pl].SetPosition(pointLights[pl].position);
			LightManager::pointLights[pl].SetConstant(pointLights[pl].constant);
			LightManager::pointLights[pl].SetLinear(pointLights[pl].linear);
			LightManager::pointLights[pl].SetExponent(pointLights[pl].exponent);
		}

		for (unsigned int sl = 0; sl < LightManager::spotLightCount; sl++)
		{
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
	}
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneSponza::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags =
		ImGuiDockNodeFlags_None |
		ImGuiDockNodeFlags_PassthruCentralNode |
		ImGuiDockNodeFlags_NoDockingInCentralNode;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", p_open, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
	{
		ImGui::PopStyleVar(2);
	}

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	else
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
		ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::SmallButton("click here"))
		{
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		}
	}

	RenderImGuiMenu(mainWindow, dockspace_flags);

	ImGui::End();
}

void SceneSponza::RenderImGuiMenu(Window* mainWindow, ImGuiDockNodeFlags dockspaceFlags)
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit")) mainWindow->SetShouldClose(true);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Undo");
			ImGui::MenuItem("Redo");
			ImGui::MenuItem("Cut");
			ImGui::MenuItem("Copy");
			ImGui::MenuItem("Paste");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Lights", "Ctrl+S"))
			{
				m_ShowWindowLights = !m_ShowWindowLights;
			}

			if (ImGui::MenuItem("Scene Settings", "Ctrl+S"))
			{
				m_ShowWindowSceneSettings = !m_ShowWindowSceneSettings;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Docking"))
		{
			// Disabling fullscreen would allow the window to be moved to the front of other windows, 
			// which we can't undo at the moment without finer window depth/z control.
			//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

			if (ImGui::MenuItem("Flag: NoSplit", "", (dockspaceFlags & ImGuiDockNodeFlags_NoSplit) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoSplit;
			if (ImGui::MenuItem("Flag: NoResize", "", (dockspaceFlags & ImGuiDockNodeFlags_NoResize) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoResize;
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspaceFlags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_PassthruCentralNode;
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspaceFlags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_AutoHideTabBar;
			ImGui::EndMenu();
		}

		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
				" > if io.ConfigDockingWithShift==false (default):" "\n"
				"   drag windows from title bar to dock" "\n"
				" > if io.ConfigDockingWithShift==true:" "\n"
				"   drag windows from anywhere and hold Shift to dock" "\n\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorateyour main //   application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the centralpoint of //  your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame( call.	An //easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		ImGui::EndMenuBar();
	}
}

void SceneSponza::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	/* Sponza scene */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.008f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));

    if (passType == "main")
    {
	    materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
    }

	models["sponza"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	if (passType == "main")
	{
		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 16.0f, -0.25f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.2f));

		shaders["main"]->SetMat4("model", model);

		shaders["main"]->SetInt("albedoMap", textureSlots["shadow"]);
		shaders["main"]->SetInt("normalMap", textureSlots["shadow"]);

		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();
	}
}

void SceneSponza::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	if (!sceneSettings.enableWaterEffects) return;

    H2M::RefH2M<MoravaShader> shaderWater = shaders["water"];

	/* Water Tile */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, m_WaterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(20.0f, 0.0f, 20.0f));

	shaderWater->Bind();
	shaderWater->SetMat4("model", model);

	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);

	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	shaderWater->SetInt("reflectionTexture", textureSlots["reflection"]);
	shaderWater->SetFloat3("lightColor", LightManager::directionalLight.GetColor());

	meshes["water"]->Render();
}

SceneSponza::~SceneSponza()
{
}
