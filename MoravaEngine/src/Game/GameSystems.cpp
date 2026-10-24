#include "GameSystems.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"

#include "Audio/AudioEngine.h"

#include "Core/Log.h"

#include "Input/InputSystem.h"

#include "Project/Project.h"

#include "Scripting/ScriptHost.h"

#include <glm/glm.hpp>

#include <cmath>
#include <filesystem>
#include <system_error>


void GameSystems::Begin(EnvMapVulkanScene* scene, InputSystem* input)
{
	m_Scripts.Stop();
	m_Scene = scene;
	m_Input = input;
	m_Physics.Stop();
	m_Audio.Stop();
	AudioEngine::SetPaused(false);

	// The scripts' API on this game (a fresh clock), or on none
	m_ScriptContext = ScriptContext();
	m_ScriptContext.Scene = scene;
	m_ScriptContext.Physics = &m_Physics;
	m_ScriptContext.Audio = &m_Audio;
	m_ScriptContext.Input = input;
	m_ScriptContext.FixedDeltaTime = GameLoop::FixedStep;
	ScriptBindings::Bind();
	ScriptBindings::SetContext(scene ? &m_ScriptContext : nullptr);

	if (m_Scene)
	{
		// The controllers the game can read (in the log: "why doesn't my gamepad work?")
		if (m_Input)
		{
			for (const InputGamepadInfo& gamepad : InputSystem::GetGamepads())
			{
				Log::GetLogger()->info("Input: gamepad {0}: {1}{2}", gamepad.Index + 1, gamepad.Name,
					gamepad.Mapped ? "" : " (no layout known: not read)");
			}
		}

		m_Scene->Each<InputTestComponent>([](EnvMapVulkanEntityID, InputTestComponent& test)
		{
			test.Started = false;
			test.VerticalSpeed = 0.0f;
		});

		// The project's collision layers (no project: the engine's)
		const Project* project = Project::GetOpen();
		m_Physics.Start(*m_Scene, project ? project->Collisions : PhysicsCollisionMatrix());

		// The sounds, mixed at the project's volumes, and the scripts after the bodies and the sounds: OnCreate may ask for
		// its Rigidbody or play a sound (not in Simulate: no game logic there, and no sound)
		if (m_Input)
		{
			AudioEngine::SetVolumes(project ? project->Volumes : AudioVolumes());
			m_Audio.Start(*m_Scene);
			m_Scripts.Start(*m_Scene);
		}
	}
}

void GameSystems::LoadScripts()
{
	if (ScriptHost::IsGameAssemblyLoaded() && ScriptHost::UnloadGameAssembly() == ScriptUnloadResult::Refused)
	{
		return; // C# logged why
	}
	const Project* project = Project::GetOpen();
	const std::string path = project ? project->GetScriptAssemblyPath() : std::string();
	if (path.empty() || !ScriptHost::IsAvailable())
	{
		return;
	}
	std::error_code error;
	if (!std::filesystem::exists(std::filesystem::u8path(path), error))
	{
		Log::GetLogger()->warn("Scripts: the project's script assembly '{0}' doesn't exist yet: build the project's scripts (dotnet build; "
			"the editor loads them once they are built)", path);
		return;
	}
	ScriptHost::LoadGameAssembly(std::filesystem::u8path(path));
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
	const EnvMapVulkanEntityID camera = EnvMapVulkanSceneData::FindPrimaryCamera(scene);
	if (camera != NoEntity)
	{
		yaw = glm::radians(scene.Get<TransformComponent>(camera).Rotation.y);
	}
	forward = glm::vec3(-std::sin(yaw), 0.0f, -std::cos(yaw));
	right = glm::vec3(std::cos(yaw), 0.0f, -std::sin(yaw));
}

void GameSystems::FixedUpdate(float fixedStep)
{
	m_ScriptContext.InFixedStep = true;
	m_ScriptContext.DeltaTime = fixedStep;
	m_ScriptContext.FixedDeltaTime = fixedStep;
	m_ScriptContext.Time += fixedStep;
	m_ScriptContext.FixedStepCount++;
	m_Scripts.RemoveDestroyed(); // the editor may have removed an entity

	// The game's logic (not in Simulate: no input)
	if (m_Scene && m_Input)
	{
		// The input test: Move, Jump and a fall back to the ground (its height when the game started)
		const InputActionState& move = m_Input->Get("Move");
		const InputActionState& jump = m_Input->Get("Jump");
		glm::vec3 forward, right;
		GetCameraGroundAxes(*m_Scene, forward, right);

		m_Scene->Each<InputTestComponent, TransformComponent>([&](EnvMapVulkanEntityID entity, InputTestComponent& test, TransformComponent& transform)
		{
			// A character: the input says how it wants to move, physics where it can (walls, slopes, steps, the ground)
			ScenePhysics::CharacterState character;
			if (m_Physics.GetCharacterState(entity, character))
			{
				const glm::vec3 horizontal = (right * move.Value.x + forward * move.Value.y) * test.Speed;
				float vertical = character.Velocity.y;
				if (character.Ground == ScenePhysics::CharacterGround::OnGround)
				{
					vertical = jump.PressedSinceFixed ? test.JumpSpeed : 0.0f;
				}
				else
				{
					vertical -= test.Gravity * fixedStep; // in the air or sliding down a steep slope
				}
				m_Physics.SetCharacterVelocity(entity, glm::vec3(horizontal.x, vertical, horizontal.z));
				return;
			}

			// Without one: moved directly, through anything, falling back to its height when the game started
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

	m_Scripts.FixedUpdate(fixedStep);

	// What the game destroyed in this step is gone before physics steps (its bodies with it)
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();

	// Physics, after what moves kinematic bodies (the input test, the scripts), then its collisions and triggers to the
	// scripts (still the fixed step: Input's edges are the step's), and what they destroyed (a pickup: gone in this frame)
	m_Physics.Step(fixedStep);
	m_Scripts.DispatchEvents(m_Physics.GetEvents());
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();
	m_ScriptContext.InFixedStep = false;

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

	m_ScriptContext.DeltaTime = frameTime;
	m_ScriptContext.FrameCount++;
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

	m_Scripts.RemoveDestroyed();
	m_Scripts.Update(frameTime);

	// What the game destroyed in this frame
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();

	// The listener and the sounds where the frame left their entities (the sounds of destroyed ones go)
	m_Audio.Update(frameTime);
}
