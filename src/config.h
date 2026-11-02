#pragma once

// The graphics APIs are switched on in CMake (MoravaEngine/CMakeLists.txt), any combination: MORAVA_VULKAN, MORAVA_OPENGL
// and MORAVA_DIRECTX11 define GRAPHICS_API_VULKAN, GRAPHICS_API_OPENGL and GRAPHICS_API_DIRECTX11. Only the classes of
// those APIs are compiled, and all their scenes: code that names a class of one API stands inside
// #if defined(GRAPHICS_API_...). Which API runs is decided at startup (main.cpp, RendererAPI_H2M::Current()).
//   cmake -DMORAVA_OPENGL=ON -DMORAVA_VULKAN=OFF -DMORAVA_DIRECTX11=OFF build     the OpenGL classes and scenes
//   cmake -DMORAVA_VULKAN=ON -DMORAVA_OPENGL=OFF -DMORAVA_DIRECTX11=OFF build     the Vulkan classes and scenes
//
// The scene that runs is chosen below, one per graphics API, by default the first scene of its row in the list. The first
// compiled API in the order Vulkan, OpenGL, DirectX 11 runs its scene. CMake's MORAVA_SCENE setting chooses the scene
// instead, when set (cmake -DMORAVA_SCENE=EDITOR build; cmake -DMORAVA_SCENE= build goes back to this choice).
//   Vulkan:      SCENE_ENV_MAP_VULKAN, SCENE_HAZEL_VULKAN
//   OpenGL:      SCENE_COTTAGE, SCENE_EIFFEL, SCENE_SPONZA, SCENE_TERRAIN, SCENE_PBR, SCENE_LEARN_OPENGL, SCENE_JOLT,
//                SCENE_INSTANCED, SCENE_ASTEROIDS, SCENE_NANOSUIT, SCENE_FRAMEBUFFERS, SCENE_CUBEMAPS, SCENE_PARTICLES,
//                SCENE_OMNI_SHADOWS, SCENE_VOXEL_TERRAIN, SCENE_PROCEDURAL_LANDMASS, SCENE_VOXEL_TERRAIN_SL,
//                SCENE_MARCHING_CUBES, SCENE_SSAO, SCENE_BLOOM, SCENE_DEFERRED, SCENE_DEFERRED_OGL, SCENE_ANIM_PBR,
//                SCENE_EDITOR, SCENE_EDITOR_IMGUIZMO, SCENE_HAZEL_ENV_MAP
//   DirectX 11:  SCENE_DX11 (Windows only)

#if !defined(MORAVA_SCENE_FROM_CMAKE)
	#if defined(GRAPHICS_API_VULKAN)
		#define SCENE_ENV_MAP_VULKAN
	#elif defined(GRAPHICS_API_OPENGL)
		#define SCENE_SSAO
	#elif defined(GRAPHICS_API_DIRECTX11)
		#define SCENE_DX11
	#endif
#endif

// BEGIN compile the engine for the chosen graphics APIs only

#if !defined(GRAPHICS_API_VULKAN) && !defined(GRAPHICS_API_OPENGL) && !defined(GRAPHICS_API_DIRECTX11)
	#error No graphics API: build with CMake, which defines GRAPHICS_API_... from its MORAVA_VULKAN, MORAVA_OPENGL, MORAVA_DIRECTX11 settings
#endif

enum class GraphicsAPI
{
	None,
	Vulkan,
	OpenGL,
	DirectX11,
	DirectX12,
	Metal,
	WebGPU
};

// The graphics API of the scene, which has to be compiled
#if defined(SCENE_ENV_MAP_VULKAN) || defined(SCENE_HAZEL_VULKAN)
	constexpr GraphicsAPI CurrentGraphicsAPI = GraphicsAPI::Vulkan;
	#if !defined(GRAPHICS_API_VULKAN)
		#error The scene chosen in config.h is a Vulkan scene, but Vulkan is not compiled: choose a scene of a compiled API, or build with -DMORAVA_VULKAN=ON
	#endif
#elif defined(SCENE_DX11)
	constexpr GraphicsAPI CurrentGraphicsAPI = GraphicsAPI::DirectX11;
	#if !defined(GRAPHICS_API_DIRECTX11)
		#error The scene chosen in config.h is the DirectX 11 scene, but DirectX 11 is not compiled: choose a scene of a compiled API, or build with -DMORAVA_DIRECTX11=ON
	#endif
#else
	constexpr GraphicsAPI CurrentGraphicsAPI = GraphicsAPI::OpenGL;
	#if !defined(GRAPHICS_API_OPENGL)
		#error The scene chosen in config.h is an OpenGL scene (or no scene is chosen), but OpenGL is not compiled: choose a scene of a compiled API, or build with -DMORAVA_OPENGL=ON
	#endif
#endif

// END compile the engine for the chosen graphics APIs only


enum class SceneName
{
	NONE = 0,
	COTTAGE,
	EIFFEL,
	SPONZA,
	TERRAIN,
	PBR,
	LEARN_OPENGL,
	JOLT,
	INSTANCED,
	ASTEROIDS,
	NANOSUIT,
	FRAMEBUFFERS,
	CUBEMAPS,
	PARTICLES,
	OMNI_SHADOWS,
	VOXEL_TERRAIN,
	PROCEDURAL_LANDMASS,
	VOXEL_TERRAIN_SL,
	MARCHING_CUBES,
	SSAO,
	BLOOM,
	DEFERRED,
	DEFERRED_OGL,
	EDITOR,
	EDITOR_IMGUIZMO,
	ANIM_PBR,
	HAZEL_ENV_MAP,
	HAZEL_VULKAN,
	DX11,
	ENV_MAP_VULKAN
};


class Scene;
class RendererBasic;

struct SceneProperties
{
	::Scene* Scene;
	RendererBasic* Renderer;
	SceneName Name;
	// H2M::RendererAPITypeH2M RendererAPI;
};

// The scenes that use 'ultramodern' OpenGL 4.5+ functions, e.g. glCreateTextures (their headers are included where the
// scene is created: EditorApplication.cpp)
#if defined(SCENE_HAZEL_ENV_MAP) || defined(SCENE_DX11)
#define OPENGL_4_5_PLUS
#endif
