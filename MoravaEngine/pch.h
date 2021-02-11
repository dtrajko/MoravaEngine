#pragma once

// Hazel classes
#include "Hazel/Core/Base.h"
#include "Hazel/Core/Math/AABB.h"
#include "Hazel/Core/Math/Ray.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/EditorCamera.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/HazelScene.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/HazelScene.h"

// Morava classes
#include "Application.h"
#include "Log.h"
#include "Timer.h"
#include "Util.h"
#include "Input.h"
#include "ImGuiWrapper.h"
#include "Math.h"
#include "Mesh.h"
#include "Scene.h"
#include "Shader.h"
#include "Material.h"
#include "Framebuffer.h"
#include "TextureCubemap.h"
#include "ResourceManager.h"
#include "RuntimeCamera.h"

// GLM
#include <glm/gtc/type_ptr.hpp>

// C++
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>
