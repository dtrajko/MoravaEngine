#include "Scene/SceneEiffel.h"

#include "Mesh/MeshData.h"
#include "Mesh/Tile2D.h"
#include "Renderer/Renderer.h"
#include "Shader/ShaderMain.h"


SceneEiffel::SceneEiffel()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = true;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 16.0f, 28.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 2.0f;

	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(-0.8f, -1.2f, 0.8f);
	sceneSettings.lightProjectionMatrix = glm::ortho(m_OrthoLeft, m_OrthoRight, m_OrthoBottom, m_OrthoTop, m_OrthoNear, m_OrthoFar);

	sceneSettings.pointLights[0].base.enabled = false;
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.diffuseIntensity = 6.0f;

	sceneSettings.pointLights[1].base.enabled = false;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pointLights[1].position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pointLights[1].base.diffuseIntensity = 6.0f;

	sceneSettings.pointLights[2].base.enabled = false;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[2].base.diffuseIntensity = 6.0f;

	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.waterHeight = 1.0f;
	sceneSettings.waterWaveSpeed = 0.2f;

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneEiffel::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_3/right.png");
	skyboxFaces.push_back("Textures/skybox_3/left.png");
	skyboxFaces.push_back("Textures/skybox_3/top.png");
	skyboxFaces.push_back("Textures/skybox_3/bottom.png");
	skyboxFaces.push_back("Textures/skybox_3/back.png");
	skyboxFaces.push_back("Textures/skybox_3/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneEiffel::SetupTextures()
{
	textures.insert(std::make_pair("sponzaFloorDiffuse", TextureLoader::Get()->GetTexture("Textures/sponza_floor_a_diff.tga", false, false)));
	textures.insert(std::make_pair("sponzaFloorNormal",  TextureLoader::Get()->GetTexture("Textures/sponza_floor_a_ddn.tga", false, false)));
	textures.insert(std::make_pair("sponzaCeilDiffuse",  TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_diff.tga", false, false)));
	textures.insert(std::make_pair("sponzaCeilNormal",   TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_ddn.tga", false, false)));
	textures.insert(std::make_pair("water",              TextureLoader::Get()->GetTexture("Textures/water.png", false, false)));
	textures.insert(std::make_pair("pyramid",            TextureLoader::Get()->GetTexture("Textures/pyramid.png", false, false)));
}

void SceneEiffel::SetupMeshes()
{
	Mesh* cube = new Mesh();
	cube->Create(&MeshData::vertices[0], &MeshData::indices[0], MeshData::vertexCount, MeshData::indexCount);
	meshes.insert(std::make_pair("cube", cube));

	Mesh* quad = new Mesh();
	quad->Create(&MeshData::quadVertices[0], &MeshData::quadIndices[0], MeshData::quadVertexCount, MeshData::quadIndexCount);
	meshes.insert(std::make_pair("quad", quad));

	Mesh* quadLarge = new Mesh();
	quadLarge->Create(&MeshData::floorVertices[0], &MeshData::floorIndices[0], MeshData::floorVertexCount, MeshData::floorIndexCount);
	meshes.insert(std::make_pair("quadLarge", quadLarge));

	Tile2D* m_Tile2D = new Tile2D();
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void SceneEiffel::SetupModels()
{ 
	Model* eiffel = new Model("Models/Eiffel_Tower.obj");
	models.insert(std::make_pair("eiffel", eiffel));

	Model* watchtower = new Model("Models/wooden_watch_tower.obj");
	models.insert(std::make_pair("watchtower", watchtower));

	Model* helicopter = new Model("Models/OGLdev/hheli/hheli.obj", "Textures/OGLdev/hheli");
	models.insert(std::make_pair("helicopter", helicopter));
}

void SceneEiffel::Update(float timestep, Window* mainWindow)
{
	Scene::Update(timestep, mainWindow);
}

void SceneEiffel::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	// BEGIN Settings ImGui Window
	{
		// Shadow rotation
		m_LightDirection = sceneSettings.directionalLight.direction;
		m_LightColor = LightManager::directionalLight.GetColor();

		float lightRadius = abs(m_LightDirection.x);
		float lightAngle = timestep * sceneSettings.shadowSpeed;
		m_LightDirection.x = (float)cos(lightAngle) * lightRadius;
		m_LightDirection.z = (float)sin(lightAngle) * lightRadius;

		if (m_ShowWindowSettings)
		{
			ImGui::Begin("Settings", &m_ShowWindowSettings);
			{
				if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Water Level");
					if (ImGui::DragFloat(" ", &sceneSettings.waterHeight, 0.1f, -2.0f, 20.0f, "%.2f"))
					{
						m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);
					}

					ImGui::Text("DirLight Color");
					if (ImGui::ColorEdit3("", glm::value_ptr(m_LightColor)))
					{
						LightManager::directionalLight.SetColor(m_LightColor);
					}
				}

				ImGui::Separator();

				ImGui::Text("Light Projection Matrix (Ortho)");

				bool changedLeft = ImGui::DragFloat("Left", &m_OrthoLeft, 0.1f, -1000.0f, 0.0f, "%.2f");
				bool changedRight = ImGui::DragFloat("Right", &m_OrthoRight, 0.1f, 0.0f, 1000.0f, "%.2f");
				bool changedBottom = ImGui::DragFloat("Bottom", &m_OrthoBottom, 0.1f, -1000.0f, 0.0f, "%.2f");
				bool changedTop = ImGui::DragFloat("Top", &m_OrthoTop, 0.1f, 0.0f, 1000.0f, "%.2f");
				bool changedNear = ImGui::DragFloat("Near", &m_OrthoNear, 0.1f, -100.0f, 0.0f, "%.2f");
				bool changedFar = ImGui::DragFloat("Far", &m_OrthoFar, 0.1f, 0.0f, 1000.0f, "%.2f");

				if (changedLeft || changedRight || changedBottom || changedTop || changedNear || changedFar)
				{
					glm::mat4 lightOrtho = glm::ortho(m_OrthoLeft, m_OrthoRight, m_OrthoBottom, m_OrthoTop, m_OrthoNear, m_OrthoFar);
					LightManager::directionalLight.SetLightProjection(lightOrtho);
				}
			}
			ImGui::End();
		}

		LightManager::directionalLight.SetDirection(m_LightDirection);
	}
	// END Settings ImGui Window

	if (m_ShowWindowFramebuffers)
	{
		ImGui::Begin("Framebuffers", &m_ShowWindowFramebuffers);
		{
			if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImVec2 imageSize(96.0f, 96.0f);

				ImGui::Text("Shadow Map");
				ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);

				ImGui::Text("Water Reflection\nColor Attachment");
				ImGui::Image((void*)(intptr_t)m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->GetID(), imageSize);
				ImGui::Text("Water Refraction\nColor Attachment");
				ImGui::Image((void*)(intptr_t)m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->GetID(), imageSize);
				ImGui::Text("Water Refraction\nDepth Attachment");
				ImGui::Image((void*)(intptr_t)m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->GetID(), imageSize);
			}
		}
		ImGui::End();
	}
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneEiffel::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
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

void SceneEiffel::RenderImGuiMenu(Window* mainWindow, ImGuiDockNodeFlags dockspaceFlags)
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
			if (ImGui::MenuItem("Settings", "Ctrl+S"))
			{
				m_ShowWindowSettings = !m_ShowWindowSettings;
			}

			if (ImGui::MenuItem("Framebuffers", "Ctrl+F"))
			{
				m_ShowWindowFramebuffers = !m_ShowWindowFramebuffers;
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

void SceneEiffel::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	H2M::RefH2M<ShaderMain> shaderMain = shaders["main"];

	glm::mat4 model;

	/* Floor */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(3.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	// shaderMain->SetMat4("model", model);
	// shaderMain->SetMat4("view", m_Camera->GetViewMatrix());
	textures["sponzaFloorDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["sponzaFloorNormal"]->Bind(textureSlots["normal"]);

	if (passType == "main")
	{
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	}

	if (passType != "shadow")
	{
		meshes["quadLarge"]->Render();
	}

	/* Eiffel model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.0003f, 0.0003f, 0.0003f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	// shaderMain->SetMat4("model", model);
	// shaderMain->SetMat4("view", m_Camera->GetViewMatrix());

	if (passType == "main")
	{
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	}

	models["eiffel"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Watchtower model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -0.35f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	// shaderMain->SetMat4("model", model);
	// shaderMain->SetMat4("view", m_Camera->GetViewMatrix());

	if (passType == "main")
	{
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	}

	models["watchtower"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Helicopter model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 1.5f, 6.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.02f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	// shaderMain->SetMat4("model", model);
	// shaderMain->SetMat4("view", m_Camera->GetViewMatrix());

	if (passType == "main")
	{
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	}

	models["helicopter"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Cube */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 5.0f, -5.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	// shaderMain->SetMat4("model", model);
	// shaderMain->SetMat4("view", m_Camera->GetViewMatrix());
	textures["pyramid"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);

	if (passType == "main")
	{
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	}

	meshes["cube"]->Render();

	if (passType == "main")
	{
		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-12.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(-5.0f, 1.0f, 5.0f * (9.0f / 16.0f)));
		shaderMain->SetMat4("model", model);
		shaderMain->SetMat4("view", m_Camera->GetViewMatrix());
		shaderMain->SetInt("albedoMap", textureSlots["shadow"]);
		shaderMain->SetInt("normalMap", textureSlots["shadow"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();

		/* Water reflection framebuffer */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(-5.0f, 1.0f, 5.0f * (9.0f / 16.0f)));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		// shaderMain->SetMat4("model", model);
		// shaderMain->SetMat4("view", m_Camera->GetViewMatrix());
		shaderMain->SetInt("albedoMap", textureSlots["diffuse"]);
		shaderMain->SetInt("normalMap", textureSlots["normal"]);
		m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();

		/* Water refraction framebuffer */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(-5.0f, 1.0f, 5.0f * (9.0f / 16.0f)));
		shaderMain->SetMat4("model", model);
		shaderMain->SetMat4("view", m_Camera->GetViewMatrix());
		shaderMain->SetInt("albedoMap", textureSlots["diffuse"]);
		shaderMain->SetInt("normalMap", textureSlots["normal"]);
		m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();
	}
}

void SceneEiffel::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	if (!sceneSettings.enableWaterEffects) return;

	H2M::RefH2M<MoravaShader> shaderWater = shaders["water"];

	/* Water Tile */
	shaderWater->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, m_WaterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(30.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	// shaderWater->SetMat4("model", model);
	// shaderWater->SetMat4("view", m_Camera->GetViewMatrix());
	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	textures["waterNormal"]->Bind(textureSlots["normal"]);
	shaderWater->SetInt("reflectionTexture", textureSlots["reflection"]);
	shaderWater->SetInt("refractionTexture", textureSlots["refraction"]);
	shaderWater->SetInt("normalMap", textureSlots["normal"]);
	shaderWater->SetInt("depthMap", textureSlots["depth"]);
	shaderWater->SetInt("dudvMap", textureSlots["DuDv"]);

	shaderWater->SetFloat3("lightColor", LightManager::directionalLight.GetColor());
	shaderWater->SetFloat3("lightPosition", -m_LightDirection);
	shaderWater->SetFloat3("eyePosition", m_Camera->GetPosition());
	shaderWater->SetFloat("waterLevel", sceneSettings.waterHeight);
	shaderWater->SetFloat4("waterColor", glm::vec4(0.0f, 0.4f, 0.8f, 1.0f));

	// shaderWater->SetFloat("nearPlane", sceneSettings.nearPlane);
	// shaderWater->SetFloat("farPlane", sceneSettings.farPlane);

	meshes["water"]->Render();

	shaderWater->Unbind();
}

SceneEiffel::~SceneEiffel()
{
}
