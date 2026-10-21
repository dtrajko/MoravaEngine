#include "GameSystems.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Core/Log.h"

#include "Input/InputSystem.h"

#include "Project/Project.h"

#include <glm/glm.hpp>

#include <cmath>


void GameSystems::Begin(EnvMapVulkanScene* scene, InputSystem* input)
{
	m_Scene = scene;
	m_Input = input;
	m_Physics.Stop();
	if (m_Scene)
	{
		// The controllers the game can read (in the log: "why doesn't my gamepad work?")
		for (const InputGamepadInfo& gamepad : InputSystem::GetGamepads())
		{
			Log::GetLogger()->info("Input: gamepad {0}: {1}{2}", gamepad.Index + 1, gamepad.Name,
				gamepad.Mapped ? "" : " (no layout known: not read)");
		}

		m_Scene->Each<InputTestComponent>([](EnvMapVulkanEntityID, InputTestComponent& test)
		{
			test.Started = false;
			test.VerticalSpeed = 0.0f;
		});

		m_Physics.Start(*m_Scene);
	}
}

InputMap GameSystems::LoadInputMap()
{
	const Project* project = Project::GetOpen();
	const std::string path = project ? project->GetInputMapPath() : std::string();
	if (path.empty())
	{
		return InputMap::Default();
	}
	InputMap map;
	std::string error;
	if (!InputMap::Load(path, map, error))
	{
		Log::GetLogger()->error("Input file '{0}' could not be loaded ({1}): the default actions instead", path, error);
		return InputMap::Default();
	}
	Log::GetLogger()->info("Input: {0} action(s) from '{1}'", map.Actions.size(), path);
	return map;
}

// Forward and right on the ground, as the scene's camera sees it (its turn around the vertical axis: 0 looks along -Z)
static void GetCameraGroundAxes(EnvMapVulkanScene& scene, glm::vec3& forward, glm::vec3& right)
{
	float yaw = 0.0f;
	const EnvMapVulkanEntityID camera = scene.FindFirst<CameraComponent>();
	if (camera != NoEntity)
	{
		yaw = glm::radians(scene.Get<TransformComponent>(camera).Rotation.y);
	}
	forward = glm::vec3(-std::sin(yaw), 0.0f, -std::cos(yaw));
	right = glm::vec3(std::cos(yaw), 0.0f, -std::sin(yaw));
}

void GameSystems::FixedUpdate(float fixedStep)
{
	if (m_Scene && m_Input)
	{
		// The input test: Move, Jump and a fall back to the ground (its height when the game started)
		const InputActionState& move = m_Input->Get("Move");
		const InputActionState& jump = m_Input->Get("Jump");
		glm::vec3 forward, right;
		GetCameraGroundAxes(*m_Scene, forward, right);

		m_Scene->Each<InputTestComponent, TransformComponent>([&](EnvMapVulkanEntityID, InputTestComponent& test, TransformComponent& transform)
		{
			if (!test.Started)
			{
				test.GroundHeight = transform.Translation.y;
				test.Started = true;
			}
			transform.Translation += (right * move.Value.x + forward * move.Value.y) * test.Speed * fixedStep;

			const bool grounded = transform.Translation.y <= test.GroundHeight;
			if (grounded && jump.PressedSinceFixed)
			{
				test.VerticalSpeed = test.JumpSpeed;
			}
			if (!grounded || test.VerticalSpeed > 0.0f)
			{
				test.VerticalSpeed -= test.Gravity * fixedStep;
				transform.Translation.y += test.VerticalSpeed * fixedStep;
				if (transform.Translation.y <= test.GroundHeight)
				{
					transform.Translation.y = test.GroundHeight;
					test.VerticalSpeed = 0.0f;
				}
			}
		});
	}

	// Physics, after what moves kinematic bodies (the input test now, scripts in Phase 4)
	m_Physics.Step(fixedStep);

	// The fixed step saw the input's edges: the next one sees only newer ones
	if (m_Input)
	{
		m_Input->EndFixedStep();
	}
}

void GameSystems::Update(float frameTime, float alpha)
{
	// The bodies where physics has them, between its last two steps
	m_Physics.WriteTransforms(alpha);

	if (!m_Scene || !m_Input)
	{
		return;
	}

	// The input test: Look turns it (Look is a rate: the mouse's pixels per second, times the frame time)
	const InputActionState& look = m_Input->Get("Look");
	// The mouse turns it only while captured: else moving to a button of the editor (or out of the window) would turn it
	const InputAction* lookAction = m_Input->GetMap().Find("Look");
	const bool lookByMouse = lookAction && look.Binding >= 0 && look.Binding < (int)lookAction->Bindings.size() &&
		lookAction->Bindings[look.Binding].Source == InputBindingSource::MouseDelta;
	const float turn = (lookByMouse && !m_Input->IsMouseCaptured()) ? 0.0f : look.Value.x;
	bool captureMouse = false;
	m_Scene->Each<InputTestComponent, TransformComponent>([&](EnvMapVulkanEntityID, InputTestComponent& test, TransformComponent& transform)
	{
		transform.Rotation.y -= turn * test.TurnSpeed * frameTime; // right: clockwise, seen from above
		captureMouse |= test.CaptureMouse;
	});
	m_Input->SetMouseCaptureWanted(captureMouse);
}
