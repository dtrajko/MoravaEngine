#include "main.h"


// Window properties
const char* WINDOW_TITLE = "Morava Engine";
const int WIDTH = 1280;
const int HEIGHT = 720;

Scene* scene;
RendererBasic* renderer;


int main()
{
	Log::Init();

	Hazel::RendererAPI::SetAPI(Hazel::RendererAPIType::OpenGL);

	std::string windowTitle = WINDOW_TITLE;
	if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::OpenGL) {
		windowTitle += " [Renderer: OpenGL]";
	}
	else if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::Vulkan) {
		windowTitle += " [Renderer: Vulkan]";
	}

	Application::Get()->InitWindow(WindowProps(windowTitle, WIDTH, HEIGHT));

#if defined(SCENE_COTTAGE)
	scene = new SceneCottage();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_EIFFEL)
	scene = new SceneEiffel();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_SPONZA)
	scene = new SceneSponza();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_TERRAIN)
	scene = new SceneTerrain();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_PBR)
	scene = new ScenePBR();
	renderer = static_cast<RendererBasic*>(new RendererPBR());
#elif defined(SCENE_LEARN_OPENGL)
	scene = new SceneJoey();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_BULLET)
	scene = new SceneBullet();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_INSTANCED)
	scene = new SceneInstanced();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_ASTEROIDS)
	scene = new SceneAsteroids();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_NANOSUIT)
	scene = new SceneNanosuit();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_FRAMEBUFFERS)
	scene = new SceneFramebuffers();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_CUBEMAPS)
	scene = new SceneCubemaps();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_PARTICLES)
	scene = new SceneParticles();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_OMNI_SHADOWS)
	scene = new SceneOmniShadows();
	renderer = static_cast<RendererBasic*>(new RendererOmniShadows());
#elif defined(SCENE_PROCEDURAL_LANDMASS)
	scene = new SceneProceduralLandmass();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_VOXEL_TERRAIN)
	scene = new SceneVoxelTerrain();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_VOXEL_TERRAIN_SL)
	scene = new SceneVoxelTerrainSL();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_MARCHING_CUBES)
	scene = new SceneMarchingCubes();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_SSAO)
	scene = new SceneSSAO();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_BLOOM)
	scene = new SceneBloom();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_ANIM_PBR)
	scene = new SceneAnimPBR();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_DEFERRED)
	scene = new SceneDeferred();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_DEFERRED_OGL)
	scene = new SceneDeferredOGL();
	renderer = static_cast<RendererBasic*>(new RendererDeferredOGL());
#elif defined(SCENE_EDITOR)
	scene = new SceneEditor();
	renderer = static_cast<RendererBasic*>(new RendererEditor());
#elif defined(SCENE_EDITOR_IMGUIZMO)
	scene = new SceneEditorImGuizmo();
	renderer = static_cast<RendererBasic*>(new RendererEditor());
#elif defined(SCENE_HAZEL_ENV_MAP)
	scene = new SceneHazelEnvMap();
	renderer = static_cast<RendererBasic*>(new RendererECS());
#elif defined(SCENE_HAZEL_VULKAN)
	scene = new SceneHazelVulkan();
	renderer = static_cast<RendererBasic*>(new RendererHazelVulkan());
#else
	throw std::runtime_error("Scene and Renderer could not be loaded!");
#endif

	Application::Get()->SetScene(scene);
	Application::Get()->SetRenderer(renderer);

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		(float)Application::Get()->GetWindow()->GetWidth() / (float)Application::Get()->GetWindow()->GetHeight(),
		Scene::GetSettings().nearPlane, Scene::GetSettings().farPlane);

	RendererBasic::SetProjectionMatrix(projectionMatrix);

	scene->SetCamera();
	scene->SetLightManager();
	scene->SetWaterManager((int)Application::Get()->GetWindow()->GetWidth(), (int)Application::Get()->GetWindow()->GetHeight());

	renderer->Init(scene);

	ImGuiWrapper::Init(Application::Get()->GetWindow());

	float targetFPS = 60.0f;
	float targetUpdateRate = 24.0f;
	Timer timer(targetFPS, targetUpdateRate);

	Application::Init();

	Application::Run();

	ImGuiWrapper::Cleanup();

	// delete scene;
	delete renderer;

	return 0;
}
