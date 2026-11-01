#pragma once

#include <glm/glm.hpp>

#include <cstdint>


// The two function tables between C++ and C# (docs/engine/Game_Engine_Runtime_Guide.html, 4.3): plain C functions,
// blittable arguments only, strings as UTF-16 with a length. Same order and layout as NativeApi, ManagedApi and the structs
// in ScriptCore/Interop.cs; a change here is a change there. glm::vec3 is System.Numerics.Vector3 (three floats); a matrix
// is glm's column-major mat4, which is System.Numerics.Matrix4x4 in memory (its row vectors are glm's columns).

struct ScriptQuat
{
	float X = 0.0f, Y = 0.0f, Z = 0.0f, W = 1.0f; // System.Numerics.Quaternion's order (glm's constructor takes w first)
};

// An input action as a script reads it. Pressed and Released are the fixed step's edges in OnFixedUpdate, the frame's in
// OnUpdate (InputSystem: ...SinceFixed or ...ThisFrame)
struct ScriptInputAction
{
	glm::vec2 Value = glm::vec2(0.0f);
	int32_t Held = 0, Pressed = 0, Released = 0;
	int32_t Exists = 0; // the input map has it
	int32_t Gamepad = 0; // the value is a gamepad's (the binding that gave it)
};

struct ScriptCharacterState
{
	int32_t Ground = 0; // ScenePhysics::CharacterGround
	glm::vec3 Velocity = glm::vec3(0.0f);
	glm::vec3 GroundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct ScriptRaycastHit
{
	uint64_t Entity = 0;
	glm::vec3 Point = glm::vec3(0.0f);
	glm::vec3 Normal = glm::vec3(0.0f);
	float Distance = 0.0f;
};

struct ScriptTime
{
	float DeltaTime = 0.0f;      // this update's: the fixed step in OnFixedUpdate, the frame's time in OnUpdate
	float FixedDeltaTime = 0.0f;
	double Time = 0.0;           // game time since the game started, in seconds (the fixed steps')
	uint64_t FrameCount = 0;
	uint64_t FixedStepCount = 0;
	float UnscaledDeltaTime = 0.0f; // the frame's real time (a fixed step's: the step)
	float TimeScale = 1.0f;         // game time per real time
	double UnscaledTime = 0.0;      // real time since the game started, in seconds
};

// An AudioSourceComponent's settings as a script reads and writes them (AudioSource's properties). Bus: AudioBus
struct ScriptAudioSettings
{
	float Volume = 1.0f, Pitch = 1.0f;
	float MinDistance = 1.0f, MaxDistance = 30.0f;
	int32_t Bus = 1;
	int32_t Loop = 0, Spatial = 1;
	int32_t Reserved = 0;
};

// A CameraComponent's settings as a script reads and writes them (Camera's properties)
struct ScriptCameraSettings
{
	int32_t Primary = 1;
	float FieldOfView = 45.0f;
	float NearClip = 0.01f, FarClip = 1000.0f;
	// A render texture camera's (7.5.4)
	int32_t Enabled = 1;
	uint32_t DrawEvery = 1;
};

// A UIRectComponent as a script reads and writes it (UIRect's properties)
struct ScriptUIRect
{
	glm::vec2 AnchorMin = glm::vec2(0.5f);
	glm::vec2 AnchorMax = glm::vec2(0.5f);
	glm::vec2 Pivot = glm::vec2(0.5f);
	glm::vec2 Position = glm::vec2(0.0f);
	glm::vec2 Size = glm::vec2(100.0f, 30.0f);
	int32_t Visible = 1;
	int32_t Reserved = 0;
};

// A UITextComponent's look (UIText's properties; the text and the font come apart, UIText_GetText...)
struct ScriptUITextStyle
{
	glm::vec4 Color = glm::vec4(1.0f);
	float Size = 32.0f;
	float LineSpacing = 1.0f;
	int32_t Align = 1;         // UITextAlign
	int32_t VerticalAlign = 1; // UITextVerticalAlign
	int32_t Wrap = 0;
	int32_t Reserved = 0;
};

// A UISliderComponent as a script reads and writes it (UISlider's properties)
struct ScriptUISlider
{
	float Value = 0.5f;
	float Min = 0.0f, Max = 1.0f;
	float Step = 0.0f;
	int32_t WholeNumbers = 0;
	int32_t Interactable = 1;
};

// What an animator does now (Animator's properties; its state's name comes apart, Animator_GetState)
struct ScriptAnimatorInfo
{
	float StateTime = 0.0f;      // seconds played in the current state (times its speed)
	float NormalizedTime = 0.0f; // 1 at its clip's end
	float FadeProgress = 0.0f;   // 0 to 1 while InTransition
	int32_t InTransition = 0;
};

// A script field's value between C++ and C# (ScriptField.h has the engine's own ScriptFieldValue). Type: ScriptFieldType.
// Text is UTF-16: a String's characters, a Prefab's file (relative to the project). GetField: C++ gives Text room for TextCapacity characters, C# writes what fits
// and sets TextLength to the string's full length.
struct ScriptValue
{
	int32_t Type = 0;
	int32_t TextLength = 0;
	int64_t Integer = 0;                // Bool (0 or 1), Int, Enum
	double Number = 0.0;                // Float, Double
	glm::vec4 Vector = glm::vec4(0.0f); // Vector2, Vector3, Vector4
	uint64_t Entity = 0;                // Entity: 0 for none
	char16_t* Text = nullptr;           // String
	int32_t TextCapacity = 0;
	int32_t Reserved = 0;
};

// One report of what the game's assembly has (DescribeScripts): a class (Name is nullptr), then each of its fields
struct ScriptFieldReport
{
	const char16_t* Class = nullptr;     // the full name, with the namespace
	const char16_t* Name = nullptr;      // the field's
	const char16_t* TypeName = nullptr;  // the C# type's: "Single", "PlayerController", "Axis"
	const char16_t* EnumNames = nullptr; // Enum: its names, a line feed between them
	const int64_t* EnumValues = nullptr; // and their values
	int32_t ClassLength = 0, NameLength = 0, TypeNameLength = 0, EnumNamesLength = 0;
	int32_t EnumCount = 0;
	int32_t Reserved = 0;
	ScriptValue Default;                 // the field's value in a new instance (a String's Text: its characters)
};

// The components a script can ask an entity for (Entity.HasComponent<T>, GetComponent<T>)
enum class ScriptComponentKind : int32_t { Rigidbody = 0, CharacterController = 1, Collider = 2, AudioSource = 3, AudioListener = 4, Camera = 5,
	Animator = 6, UIRect = 7, UIText = 8, UIImage = 9, UIButton = 10, UISlider = 11 };

// What C# may call in C++: grows with each phase. The functions about an entity return 0 when it doesn't exist (any more):
// C# turns that into an exception that names it. Only Log works outside a running game; the rest come with the game's
// scene (ScriptBindings)
struct ScriptNativeApi
{
	void (*Log)(int level, const char16_t* text, int length); // 0 info, 1 warning, 2 error

	// Entity
	int32_t (*Entity_Exists)(uint64_t entity);
	int32_t (*Entity_GetName)(uint64_t entity, char16_t* buffer, int capacity); // the length, -1: doesn't fit, -2: no entity
	int32_t (*Entity_SetName)(uint64_t entity, const char16_t* name, int length);
	int32_t (*Entity_GetTranslation)(uint64_t entity, glm::vec3* out);
	int32_t (*Entity_SetTranslation)(uint64_t entity, const glm::vec3* value);
	int32_t (*Entity_GetRotation)(uint64_t entity, ScriptQuat* out);
	int32_t (*Entity_SetRotation)(uint64_t entity, const ScriptQuat* value);
	int32_t (*Entity_GetEulerAngles)(uint64_t entity, glm::vec3* out); // degrees, as the Inspector shows them
	int32_t (*Entity_SetEulerAngles)(uint64_t entity, const glm::vec3* value);
	int32_t (*Entity_GetScale)(uint64_t entity, glm::vec3* out);
	int32_t (*Entity_SetScale)(uint64_t entity, const glm::vec3* value);
	int32_t (*Entity_GetWorldTransform)(uint64_t entity, glm::mat4* out);
	int32_t (*Entity_HasTag)(uint64_t entity, const char16_t* tag, int length); // 1 yes, 0 no, -1 no entity
	int32_t (*Entity_AddTag)(uint64_t entity, const char16_t* tag, int length);
	int32_t (*Entity_RemoveTag)(uint64_t entity, const char16_t* tag, int length);
	int32_t (*Entity_HasComponent)(uint64_t entity, int32_t component); // 1 yes, 0 no, -1 no entity
	int32_t (*Entity_Destroy)(uint64_t entity); // at the end of the update it was called in

	// Scene
	uint64_t (*Scene_FindEntityByName)(const char16_t* name, int length); // 0: none
	int32_t (*Scene_FindEntitiesWithTag)(const char16_t* tag, int length, uint64_t* out, int capacity); // how many there are

	// Input
	void (*Input_GetAction)(const char16_t* name, int length, ScriptInputAction* out);

	// Rigidbody: 0 when the entity has no dynamic body. mode: ScenePhysics::ForceMode
	int32_t (*Rigidbody_AddForce)(uint64_t entity, const glm::vec3* force, int32_t mode);
	int32_t (*Rigidbody_AddTorque)(uint64_t entity, const glm::vec3* torque, int32_t mode);
	int32_t (*Rigidbody_GetLinearVelocity)(uint64_t entity, glm::vec3* out);
	int32_t (*Rigidbody_SetLinearVelocity)(uint64_t entity, const glm::vec3* value);
	int32_t (*Rigidbody_GetAngularVelocity)(uint64_t entity, glm::vec3* out);
	int32_t (*Rigidbody_SetAngularVelocity)(uint64_t entity, const glm::vec3* value);

	// CharacterController: 0 when the entity isn't a running character
	int32_t (*Character_SetVelocity)(uint64_t entity, const glm::vec3* velocity);
	int32_t (*Character_GetState)(uint64_t entity, ScriptCharacterState* out);

	// Physics queries. layers: ObjectLayers bits (Solid, All...); ignore: an entity the query goes through (0: none).
	// The overlaps return how many entities there are (more than capacity: call again with more room)
	int32_t (*Physics_Raycast)(const glm::vec3* origin, const glm::vec3* direction, float maxDistance, uint32_t layers, uint64_t ignore,
		ScriptRaycastHit* hit);
	int32_t (*Physics_SphereCast)(const glm::vec3* origin, float radius, const glm::vec3* direction, float maxDistance, uint32_t layers,
		uint64_t ignore, ScriptRaycastHit* hit);
	int32_t (*Physics_OverlapSphere)(const glm::vec3* center, float radius, uint32_t layers, uint64_t* out, int capacity);
	int32_t (*Physics_OverlapBox)(const glm::vec3* center, const glm::vec3* halfExtents, const ScriptQuat* rotation, uint32_t layers,
		uint64_t* out, int capacity);

	// Time
	void (*Time_Get)(ScriptTime* out);

	// AudioSource (5.2): 0 when the entity has no AudioSourceComponent. Play, Pause and Stop: also 0 when the game has no
	// sound (its clip couldn't be loaded). Clips are paths relative to the project ("Assets/Audio/Pickup.wav")
	int32_t (*AudioSource_Play)(uint64_t entity);
	int32_t (*AudioSource_Pause)(uint64_t entity);
	int32_t (*AudioSource_Stop)(uint64_t entity);
	int32_t (*AudioSource_IsPlaying)(uint64_t entity); // 1 yes, 0 no, -1 no source
	int32_t (*AudioSource_GetSettings)(uint64_t entity, ScriptAudioSettings* out);
	int32_t (*AudioSource_SetSettings)(uint64_t entity, const ScriptAudioSettings* value);
	int32_t (*AudioSource_GetClip)(uint64_t entity, char16_t* buffer, int capacity); // as Entity_GetName
	int32_t (*AudioSource_SetClip)(uint64_t entity, const char16_t* clip, int length);
	// AudioListener: 0 when the entity has none
	int32_t (*AudioListener_GetActive)(uint64_t entity, int32_t* out);
	int32_t (*AudioListener_SetActive)(uint64_t entity, int32_t active);

	// Audio: a sound played once, at a place (position: as loud as volume within minDistance meters) or everywhere
	// (nullptr); 0 when its clip can't be loaded. The
	// volumes: Master, then each bus's (AudioBus order), for this game (the project's again at the next Play)
	int32_t (*Audio_PlayOneShot)(const char16_t* clip, int length, const glm::vec3* position, int32_t bus, float volume, float pitch,
		float minDistance);
	void (*Audio_GetVolumes)(float* out);
	void (*Audio_SetVolumes)(const float* volumes);

	// Camera (6.3): 0 when the entity has no CameraComponent. Primary set: the other cameras stop being primary
	int32_t (*Camera_GetSettings)(uint64_t entity, ScriptCameraSettings* out);
	int32_t (*Camera_SetSettings)(uint64_t entity, const ScriptCameraSettings* value);

	// Scene (6.2). Instantiate: the prefab file (relative to the project) placed at once under parent (0: at the top), at
	// position and rotation in the parent's space; its root, 0 when it can't be placed (logged). Load: the scene file
	// (relative to the project, ".mscene" may be left out) at the end of the frame, nullptr: the current one again; 0 when
	// there is no such file. The current scene's name and file (relative to the project): as Entity_GetName
	uint64_t (*Scene_Instantiate)(const char16_t* prefab, int length, const glm::vec3* position, const ScriptQuat* rotation, uint64_t parent);
	int32_t (*Scene_Load)(const char16_t* scene, int length);
	int32_t (*Scene_GetName)(char16_t* buffer, int capacity);
	int32_t (*Scene_GetPath)(char16_t* buffer, int capacity);

	// Animator (6.4): 0 when the entity has no animator in the running game; -1 when its controller has no parameter of that
	// name and type (type: AnimatorParameterType), or no state of that name. A trigger: value 1 sets it, 0 resets it.
	// Play: to the state at once, crossfading for fade seconds. GetState: the current state's name, as Entity_GetName
	int32_t (*Animator_SetParameter)(uint64_t entity, const char16_t* name, int length, int32_t type, float value);
	int32_t (*Animator_GetParameter)(uint64_t entity, const char16_t* name, int length, int32_t type, float* out);
	int32_t (*Animator_Play)(uint64_t entity, const char16_t* state, int length, float fade);
	int32_t (*Animator_GetInfo)(uint64_t entity, ScriptAnimatorInfo* out);
	int32_t (*Animator_GetState)(uint64_t entity, char16_t* buffer, int capacity);

	// Game UI (7.1): 0 when the entity has no such component. The text and the files (relative to the project) as
	// Entity_GetName; an empty font or texture: the engine's font, a plain rectangle
	int32_t (*UIRect_Get)(uint64_t entity, ScriptUIRect* out);
	int32_t (*UIRect_Set)(uint64_t entity, const ScriptUIRect* value);
	int32_t (*UIText_GetText)(uint64_t entity, char16_t* buffer, int capacity);
	int32_t (*UIText_SetText)(uint64_t entity, const char16_t* text, int length);
	int32_t (*UIText_GetStyle)(uint64_t entity, ScriptUITextStyle* out);
	int32_t (*UIText_SetStyle)(uint64_t entity, const ScriptUITextStyle* value);
	int32_t (*UIText_GetFont)(uint64_t entity, char16_t* buffer, int capacity);
	int32_t (*UIText_SetFont)(uint64_t entity, const char16_t* font, int length);
	int32_t (*UIImage_GetColor)(uint64_t entity, glm::vec4* out);
	int32_t (*UIImage_SetColor)(uint64_t entity, const glm::vec4* value);
	int32_t (*UIImage_GetTexture)(uint64_t entity, char16_t* buffer, int capacity);
	int32_t (*UIImage_SetTexture)(uint64_t entity, const char16_t* texture, int length);

	// Input's cursor: locked = the game wants the mouse. The state: 1 locked, 2 the mouse captured now (input enabled, and
	// the user didn't take it back)
	void (*Input_SetCursorLocked)(int32_t locked);
	int32_t (*Input_GetCursorState)();

	// Game UI (7.3). UIButton: 0 when the entity has no button. The selection (UI/SceneUI): the selected button, 0 for none;
	// UI_Select(0) drops it, and returns 0 when the button can't be selected now (hidden, not Interactable). Pointer over:
	// 1 when the mouse is over an element that catches it
	int32_t (*UIButton_GetInteractable)(uint64_t entity, int32_t* out);
	int32_t (*UIButton_SetInteractable)(uint64_t entity, int32_t interactable);
	uint64_t (*UI_GetSelected)();
	int32_t (*UI_Select)(uint64_t entity);
	int32_t (*UI_IsPointerOver)();
	// Time.TimeScale: game time per real time from the next frame on (0: standing still; below 0 counts as 0)
	void (*Time_SetTimeScale)(float scale);
	// Camera.Target (7.5.4): the render texture file (relative to the project; empty: the screen), as Entity_GetName (-2: no
	// camera). Set: 0 when the entity has no CameraComponent; a file that can't be read is logged when the camera draws
	int32_t (*Camera_GetTarget)(uint64_t entity, char16_t* buffer, int capacity);
	int32_t (*Camera_SetTarget)(uint64_t entity, const char16_t* target, int length);
	// Application.Quit (Milestone 2): the game ends after this frame (the player closes, the editor stops Play)
	void (*Application_Quit)();
	// Settings (Milestone 2): text values by key, kept between the game's runs (GameSettings). Get: as Entity_GetName,
	// -2 when there is no such key. Remove: 1 removed, 0 there was none. Save: 1 written (or nothing to write), 0 failed
	// (logged). GetFile: the file, as Entity_GetName (empty: for this run only)
	int32_t (*Settings_Get)(const char16_t* key, int keyLength, char16_t* buffer, int capacity);
	void (*Settings_Set)(const char16_t* key, int keyLength, const char16_t* value, int valueLength);
	int32_t (*Settings_Remove)(const char16_t* key, int keyLength);
	int32_t (*Settings_Save)();
	int32_t (*Settings_GetFile)(char16_t* buffer, int capacity);
	// UISlider (Milestone 2): 0 when the entity has no slider. Set: the value kept within Min and Max (rounded with
	// WholeNumbers); a value set here calls no On Value Changed and no ValueChanged listener
	int32_t (*UISlider_Get)(uint64_t entity, ScriptUISlider* out);
	int32_t (*UISlider_Set)(uint64_t entity, const ScriptUISlider* value);
};

// What C++ calls in C#; Interop.Initialize fills it in. A script instance is a GCHandle of the C# object. The Invoke
// functions return 0, or 1 when the script threw (C# has logged it)
struct ScriptManagedApi
{
	int (*GetRuntimeVersion)(char16_t* buffer, int capacity);   // the length written, -1 when it doesn't fit
	int (*GetRuntimeDirectory)(char16_t* buffer, int capacity); // the folder of the base library
	int (*LoadGameAssembly)(const char16_t* path, int length);  // 0 when loaded
	int (*UnloadGameAssembly)();                                // 0 collected, 1 still alive, -1 refused (instances left)
	void* (*CreateInstance)(const char16_t* className, int length, uint64_t entity); // nullptr: no such class, or it threw
	void (*DestroyInstance)(void* instance);
	int (*InvokeCreate)(void* instance);
	int (*InvokeUpdate)(void* instance, float dt);
	int (*InvokeFixedUpdate)(void* instance, float dt);
	// kind: ScriptCollisionKind; point and normal (from the script's entity towards the other) for CollisionEnter
	int (*InvokeCollision)(void* instance, int kind, uint64_t other, const glm::vec3* point, const glm::vec3* normal);
	// The game's script classes and their public fields, one report per call of report (user is passed through): the
	// number of classes, -1 on an error (logged)
	int (*DescribeScripts)(void* user, void (*report)(void* user, const ScriptFieldReport* field));
	// A public field of an instance, by name: 0 done, 1 not (logged: no such field, or of another type)
	int (*SetField)(void* instance, const char16_t* name, int length, const ScriptValue* value);
	int (*GetField)(void* instance, const char16_t* name, int length, ScriptValue* value);
	// An event of the entity's animator (6.4): its name and the state whose clip fired it
	int (*InvokeAnimationEvent)(void* instance, const char16_t* name, int nameLength, const char16_t* state, int stateLength);
	// A UI button's click (7.3): a method of the instance without parameters, by name (public or not): 0 called, 1 threw,
	// 2 its class has no such method
	int (*InvokeMethod)(void* instance, const char16_t* name, int length);
	// The button's Clicked event, to its C# listeners: 0, or 1 when one threw
	int (*InvokeButtonClicked)(uint64_t button);
	// The managed heap in bytes (without collecting) and the garbage collections since startup (7.4's debug overlay)
	void (*GetMemory)(int64_t* bytes, int32_t* collections);
	// A slider's On Value Changed (Milestone 2): a method of the instance with one float parameter, by name (public or
	// not): 0 called, 1 threw, 2 its class has no such method
	int (*InvokeMethodFloat)(void* instance, const char16_t* name, int length, float value);
	// The slider's ValueChanged event, to its C# listeners: 0, or 1 when one threw
	int (*InvokeSliderChanged)(uint64_t slider, float value);
};

enum class ScriptCollisionKind : int32_t { CollisionEnter = 0, CollisionExit = 1, TriggerEnter = 2, TriggerExit = 3 };
