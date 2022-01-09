#define _CRT_SECURE_NO_WARNINGS

#pragma once

// Morava classes
#include "Camera/RuntimeCamera.h"
#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "Core/ResourceManager.h"
#include "Framebuffer/MoravaFramebuffer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Scene/Scene.h"
#include "Shader/MoravaShader.h"
#include "Texture/TextureCubemap.h"

// H2M classes
#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/Events/KeyEventH2M.h"
#include "H2M/Core/Events/MouseEventH2M.h"
#include "H2M/Core/Math/AABB_H2M.h"
#include "H2M/Core/Math/RayH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Editor/EditorCameraH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"
#include "H2M/Renderer/RenderCommandQueueH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/VertexArrayH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"
#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Scene/SceneH2M.h"
#include "H2M/Scene/EntityH2M.h"

// GLM
#include <glm/gtc/type_ptr.hpp>

// C++
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>
