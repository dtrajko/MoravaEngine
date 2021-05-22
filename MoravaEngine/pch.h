#define _CRT_SECURE_NO_WARNINGS

#pragma once

// Hazel classes
#include "Hazel/Core/Base.h"
#include "Hazel/Core/Math/AABB.h"
#include "Hazel/Core/Math/Ray.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Editor/EditorCamera.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Renderer/IndexBuffer.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/VertexBuffer.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/HazelScene.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/HazelScene.h"

// Morava classes
#include "Camera/RuntimeCamera.h"
#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "Core/ResourceManager.h"
#include "Framebuffer/Framebuffer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Scene/Scene.h"
#include "Shader/Shader.h"
#include "Texture/TextureCubemap.h"

// GLM
#include <glm/gtc/type_ptr.hpp>

// C++
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>
