#include "GameSystems.h"

#include "Core/CpuStats.h"

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
	m_Scene = scene;
	m_Input = input;
	Start(false);
	if (!scene)
	{
		m_Settings.Close(); // the game is over: what changed is saved
		m_SaveData.Close();
	}
}

GameSystems::~GameSystems()
{
	ScriptBindings::SetContext(nullptr);
	m_Settings.Close();
	m_SaveData.Close();
}

void GameSystems::OpenSettings(const std::string& settingsFile, const std::string& saveDataFile)
{
	m_Settings.Open(settingsFile);
	m_SaveData.Open(saveDataFile);
}

bool GameSystems::TakeResolutionRequest(PlayerResolution& resolution)
{
	if (!m_ScriptContext.ResolutionRequested)
	{
		return false;
	}
	resolution = m_ScriptContext.ResolutionRequest;
	m_ScriptContext.ResolutionRequested = false;
	return true;
}

bool GameSystems::TakeVSyncRequest(bool& vsync)
{
	if (!m_ScriptContext.VSyncRequested)
	{
		return false;
	}
	vsync = m_ScriptContext.VSyncRequest;
	m_ScriptContext.VSyncRequested = false;
	return true;
}

bool GameSystems::TakeQualityRequest(QualityTier& tier)
{
	if (!m_ScriptContext.QualityRequested)
	{
		return false;
	}
	tier = m_ScriptContext.QualityRequest;
	m_ScriptContext.QualityRequested = false;
	return true;
}

void GameSystems::SetDisplayState(const std::vector<PlayerResolution>& resolutions, PlayerResolution resolution, bool vsync, QualityTier tier,
	bool rayTracing)
{
	m_ScriptContext.Resolutions = resolutions;
	m_ScriptContext.Resolution = resolution;
	m_ScriptContext.VSync = vsync;
	m_ScriptContext.Quality = tier;
	m_ScriptContext.RayTracingSupported = rayTracing;
}

void GameSystems::Restart()
{
	Start(true);
}

void GameSystems::SetSceneInfo(const std::string& name, const std::string& path)
{
	m_ScriptContext.SceneName = name;
	m_ScriptContext.ScenePath = path;
}

bool GameSystems::TakeQuit()
{
	const bool quit = m_ScriptContext.QuitRequested;
	m_ScriptContext.QuitRequested = false;
	return quit;
}

bool GameSystems::TakeFullscreenRequest(bool& fullscreen)
{
	if (m_ScriptContext.FullscreenRequest < 0)
	{
		return false;
	}
	fullscreen = m_ScriptContext.FullscreenRequest == 1;
	m_ScriptContext.FullscreenRequest = -1;
	return true;
}

bool GameSystems::TakeSceneLoad(std::string& path)
{
	if (!m_ScriptContext.SceneLoadRequested)
	{
		return false;
	}
	path = m_ScriptContext.SceneLoadPath;
	m_ScriptContext.SceneLoadRequested = false;
	m_ScriptContext.SceneLoadPath.clear();
	return true;
}

void GameSystems::Start(bool keepClock)
{
	m_Scripts.Stop();
	m_UI.Stop();
	m_Physics.Stop();
	m_Audio.Stop();
	m_Animation.Stop();
	AudioEngine::SetPaused(false);

	// The scripts' API on this game (a fresh clock, unless a loaded scene goes on with it), or on none. The scene's name
	// and file are the owner's (SetSceneInfo, before Begin: OnCreate may ask for them)
	ScriptContext previous = std::move(m_ScriptContext);
	m_ScriptContext = ScriptContext();
	m_ScriptContext.SceneName = std::move(previous.SceneName);
	m_ScriptContext.ScenePath = std::move(previous.ScenePath);
	// The owner's view of the window and the quality: OnCreate may ask for them before the next frame says it again
	m_ScriptContext.Fullscreen = previous.Fullscreen;
	m_ScriptContext.Resolutions = std::move(previous.Resolutions);
	m_ScriptContext.Resolution = previous.Resolution;
	m_ScriptContext.VSync = previous.VSync;
	m_ScriptContext.Quality = previous.Quality;
	m_ScriptContext.RayTracingSupported = previous.RayTracingSupported;
	if (keepClock)
	{
		m_ScriptContext.Time = previous.Time;
		m_ScriptContext.FrameCount = previous.FrameCount;
		m_ScriptContext.FixedStepCount = previous.FixedStepCount;
		m_ScriptContext.InstantiatedPrefabs = std::move(previous.InstantiatedPrefabs);
		m_ScriptContext.CursorLocked = previous.CursorLocked; // no flicker of the cursor at Scene.Reload
		m_ScriptContext.TimeScale = previous.TimeScale;
		m_ScriptContext.UnscaledTime = previous.UnscaledTime;
	}
	m_ScriptContext.Scene = m_Scene;
	m_ScriptContext.Physics = &m_Physics;
	m_ScriptContext.Audio = &m_Audio;
	m_ScriptContext.Animation = &m_Animation;
	m_ScriptContext.Scripts = &m_Scripts;
	m_ScriptContext.Input = m_Input;
	m_ScriptContext.UI = &m_UI;
	m_ScriptContext.Settings = &m_Settings;
	m_ScriptContext.SaveData = &m_SaveData;
	m_ScriptContext.ProjectInputMap = &m_ProjectInputMap;
	m_ScriptContext.FixedDeltaTime = GameLoop::FixedStep;
	ScriptBindings::Bind();
	ScriptBindings::SetContext(m_Scene ? &m_ScriptContext : nullptr);

	if (m_Scene)
	{
		// The controllers the game can read (in the log: "why doesn't my gamepad work?"), once per game
		if (m_Input && !keepClock)
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

		// The animators, in their entry states (in Simulate too: nothing sets their parameters there)
		m_Animation.Start(*m_Scene);

		// The sounds, mixed at the project's volumes, and the scripts after the bodies, the animators and the sounds:
		// OnCreate may ask for its Rigidbody, set a parameter or play a sound (not in Simulate: no game logic there, and no
		// sound)
		if (m_Input)
		{
			if (!keepClock)
			{
				// The project's volumes with the player's in place (a loaded scene keeps the game's: Audio.SetBusVolume), and the
				// project's bindings with the player's
				const PlayerSettings& player = m_Settings.GetPlayer();
				AudioEngine::SetVolumes(player.ApplyVolumes(project ? project->Volumes : AudioVolumes()));
				m_ProjectInputMap = m_Input->GetMap();
				if (!player.Bindings.empty())
				{
					InputMap map = m_ProjectInputMap;
					player.ApplyBindings(map);
					m_Input->SetMap(map);
					Log::GetLogger()->info("Input: the player's bindings of {0} action(s)", player.Bindings.size());
				}
			}
			m_Audio.Start(*m_Scene);
			m_UI.Start(*m_Scene, m_Input->GetMap());
			m_Scripts.Start(*m_Scene);
		}
	}
}

uint32_t GameSystems::UpdateUIPointer(const UIPointer& pointer, const glm::vec2& viewSize)
{
	if (m_Input && m_Input->IsRebinding())
	{
		// A rebind listens: a click is the binding (or what gives up), not a button's
		UIPointer still = pointer;
		still.Buttons = 0;
		return m_UI.UpdatePointer(still, viewSize);
	}
	return m_UI.UpdatePointer(pointer, viewSize);
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

void GameSystems::DrivePlayerVehicles()
{
	// The project's vehicle actions: Accelerate and Brake (buttons or triggers: Brake held while it rolls forward brakes,
	// then reverses), Steer (an axis: -1 left, 1 right) and Handbrake. A project without them drives with Move (forward
	// and back, left and right) and Jump (the handbrake), as a new project has them
	const InputMap& map = m_Input->GetMap();
	// The action's value (its axis: 0 x, 1 y), or the fallback's, or 0 when the project has neither
	auto value = [&](const char* action, const char* fallback, int axis) -> float
	{
		if (map.Find(action))
		{
			return m_Input->Get(action).Value[axis];
		}
		return fallback && map.Find(fallback) ? m_Input->Get(fallback).Value[axis] : 0.0f;
	};
	ScenePhysics::VehicleInput input;
	if (map.Find("Accelerate") || map.Find("Brake"))
	{
		input.Forward = value("Accelerate", nullptr, 0) - value("Brake", nullptr, 0);
	}
	else
	{
		input.Forward = value("Move", nullptr, 1);
	}
	input.Steer = map.Find("Steer") ? value("Steer", nullptr, 0) : value("Move", nullptr, 0);
	input.Handbrake = value("Handbrake", "Jump", 0);
	m_Scene->Each<VehicleComponent>([&](EnvMapVulkanEntityID entity, VehicleComponent& vehicle)
	{
		if (vehicle.Controls == VehicleComponent::ControlMode::Player)
		{
			m_Physics.SetVehicleInput(entity, input);
		}
	});
}

void GameSystems::FixedUpdate(float fixedStep)
{
	m_ScriptContext.InFixedStep = true;
	m_ScriptContext.DeltaTime = fixedStep;
	m_ScriptContext.UnscaledDeltaTime = fixedStep;
	m_ScriptContext.FixedDeltaTime = fixedStep;
	m_ScriptContext.Time += fixedStep;
	m_ScriptContext.FixedStepCount++;
	m_Scripts.RemoveDestroyed(); // the editor may have removed an entity

	// The game's logic (not in Simulate: no input). The input test's actions only with an input test in the scene: a game
	// without Move and Jump in its input map (the racing game) isn't asked for them
	if (m_Scene && m_Input && m_Scene->FindFirst<InputTestComponent>() != NoEntity)
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

	// The vehicles the player drives (VehicleComponent, Controls: Player): before the scripts, so one can still take over
	if (m_Scene && m_Input)
	{
		DrivePlayerVehicles();
	}

	CpuStats::Timer stages(CpuStats::ParentUpdate); // the game's share of a frame (the Stats panel, F3, --frame-stats)
	m_Scripts.FixedUpdate(fixedStep);
	stages.Next("Scripts' OnFixedUpdate");

	// What the game destroyed in this step is gone before physics steps (its bodies with it)
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();

	// Physics, after what moves kinematic bodies (the input test, the scripts), then its collisions and triggers to the
	// scripts (still the fixed step: Input's edges are the step's), and what they destroyed (a pickup: gone in this frame)
	m_Physics.Step(fixedStep);
	stages.Next("Physics step");
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

void GameSystems::Update(float frameTime, float unscaledFrameTime, float alpha)
{
	// The bodies where physics has them, between its last two steps
	m_Physics.WriteTransforms(alpha);

	m_ScriptContext.DeltaTime = frameTime;
	m_ScriptContext.UnscaledDeltaTime = unscaledFrameTime;
	m_ScriptContext.UnscaledTime += unscaledFrameTime;
	m_ScriptContext.FrameCount++;
	if (!m_Scene)
	{
		return;
	}
	if (!m_Input)
	{
		m_Animation.Update(frameTime); // Simulate: the animators in their entry states, as nothing sets their parameters
		return;
	}

	// A binding the player changed (Input.Rebind) is theirs from now on: all of its action's bindings are remembered
	std::string rebound;
	if (m_Input->TakeRebound(rebound))
	{
		if (const InputAction* action = m_Input->GetMap().Find(rebound))
		{
			m_Settings.EditPlayer().Bindings[rebound] = action->Bindings;
		}
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
	m_Input->SetMouseCaptureWanted(captureMouse || m_ScriptContext.CursorLocked);

	// The UI in real time (a pause menu works at time scale 0), its clicks to the scripts before their OnUpdate, and what
	// those destroyed
	m_Scripts.RemoveDestroyed();
	CpuStats::Timer stages(CpuStats::ParentUpdate); // the game's share of a frame (the Stats panel, F3, --frame-stats)
	m_UI.Update(m_Input, unscaledFrameTime);
	stages.Next("Game UI");
	m_Scripts.DispatchButtonClicks(m_UI.GetClicks());
	m_Scripts.DispatchSliderChanges(m_UI.GetValueChanges());
	m_Scripts.DispatchDropdownChanges(m_UI.GetDropdownChanges());
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();

	m_Scripts.Update(frameTime);
	stages.Next("Scripts' OnUpdate");

	// What the game destroyed in this frame
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();

	// The poses, from the parameters the scripts set (in OnUpdate, or in the fixed steps before it), then the events their
	// clips passed to the scripts (a footstep's sound: where the frame left the entity), and what those destroyed
	m_Animation.Update(frameTime);
	m_Scripts.DispatchAnimationEvents(m_Animation.GetEvents());
	ScriptBindings::DestroyQueued(m_ScriptContext);
	m_Scripts.RemoveDestroyed();

	// The listener and the sounds where the frame left their entities (the sounds of destroyed ones go)
	stages.Next("Animation");
	m_Audio.Update(frameTime);
	stages.Next("Audio");

	// A frame of no game time (time scale 0: a pause menu) runs no fixed step: its presses (A on the menu's Resume) are not
	// the game's, so they don't wait for the next step (the ball would jump when the game goes on)
	if (frameTime <= 0.0f)
	{
		m_Input->EndFixedStep();
	}
}
