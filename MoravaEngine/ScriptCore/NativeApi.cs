using System.Numerics;
using System.Runtime.InteropServices;

namespace Morava
{
    // What C# may call in C++: plain C functions, blittable arguments only, strings as UTF-16 with a length. Same order and
    // layout as ScriptNativeApi and its structs in src/Scripting/ScriptApi.h; grows with each phase. The functions about an
    // entity return 0 when it doesn't exist (any more). Only Log is there outside a running game.
    [StructLayout(LayoutKind.Sequential)]
    unsafe struct NativeApi
    {
        public delegate* unmanaged<int, char*, int, void> Log;

        public delegate* unmanaged<ulong, int> Entity_Exists;
        public delegate* unmanaged<ulong, char*, int, int> Entity_GetName;
        public delegate* unmanaged<ulong, char*, int, int> Entity_SetName;
        public delegate* unmanaged<ulong, Vector3*, int> Entity_GetTranslation;
        public delegate* unmanaged<ulong, Vector3*, int> Entity_SetTranslation;
        public delegate* unmanaged<ulong, Quaternion*, int> Entity_GetRotation;
        public delegate* unmanaged<ulong, Quaternion*, int> Entity_SetRotation;
        public delegate* unmanaged<ulong, Vector3*, int> Entity_GetEulerAngles;
        public delegate* unmanaged<ulong, Vector3*, int> Entity_SetEulerAngles;
        public delegate* unmanaged<ulong, Vector3*, int> Entity_GetScale;
        public delegate* unmanaged<ulong, Vector3*, int> Entity_SetScale;
        public delegate* unmanaged<ulong, Matrix4x4*, int> Entity_GetWorldTransform;
        public delegate* unmanaged<ulong, char*, int, int> Entity_HasTag;
        public delegate* unmanaged<ulong, char*, int, int> Entity_AddTag;
        public delegate* unmanaged<ulong, char*, int, int> Entity_RemoveTag;
        public delegate* unmanaged<ulong, int, int> Entity_HasComponent;
        public delegate* unmanaged<ulong, int> Entity_Destroy;

        public delegate* unmanaged<char*, int, ulong> Scene_FindEntityByName;
        public delegate* unmanaged<char*, int, ulong*, int, int> Scene_FindEntitiesWithTag;

        public delegate* unmanaged<char*, int, NativeInputAction*, void> Input_GetAction;

        public delegate* unmanaged<ulong, Vector3*, int, int> Rigidbody_AddForce;
        public delegate* unmanaged<ulong, Vector3*, int, int> Rigidbody_AddTorque;
        public delegate* unmanaged<ulong, Vector3*, int> Rigidbody_GetLinearVelocity;
        public delegate* unmanaged<ulong, Vector3*, int> Rigidbody_SetLinearVelocity;
        public delegate* unmanaged<ulong, Vector3*, int> Rigidbody_GetAngularVelocity;
        public delegate* unmanaged<ulong, Vector3*, int> Rigidbody_SetAngularVelocity;

        public delegate* unmanaged<ulong, Vector3*, int> Character_SetVelocity;
        public delegate* unmanaged<ulong, NativeCharacterState*, int> Character_GetState;

        public delegate* unmanaged<Vector3*, Vector3*, float, uint, ulong, NativeRaycastHit*, int> Physics_Raycast;
        public delegate* unmanaged<Vector3*, float, Vector3*, float, uint, ulong, NativeRaycastHit*, int> Physics_SphereCast;
        public delegate* unmanaged<Vector3*, float, uint, ulong*, int, int> Physics_OverlapSphere;
        public delegate* unmanaged<Vector3*, Vector3*, Quaternion*, uint, ulong*, int, int> Physics_OverlapBox;

        public delegate* unmanaged<NativeTime*, void> Time_Get;

        public delegate* unmanaged<ulong, int> AudioSource_Play;
        public delegate* unmanaged<ulong, int> AudioSource_Pause;
        public delegate* unmanaged<ulong, int> AudioSource_Stop;
        public delegate* unmanaged<ulong, int> AudioSource_IsPlaying;
        public delegate* unmanaged<ulong, NativeAudioSettings*, int> AudioSource_GetSettings;
        public delegate* unmanaged<ulong, NativeAudioSettings*, int> AudioSource_SetSettings;
        public delegate* unmanaged<ulong, char*, int, int> AudioSource_GetClip;
        public delegate* unmanaged<ulong, char*, int, int> AudioSource_SetClip;
        public delegate* unmanaged<ulong, int*, int> AudioListener_GetActive;
        public delegate* unmanaged<ulong, int, int> AudioListener_SetActive;

        public delegate* unmanaged<char*, int, Vector3*, int, float, float, int> Audio_PlayOneShot;
        public delegate* unmanaged<float*, void> Audio_GetVolumes;
        public delegate* unmanaged<float*, void> Audio_SetVolumes;

        public delegate* unmanaged<ulong, NativeCameraSettings*, int> Camera_GetSettings;
        public delegate* unmanaged<ulong, NativeCameraSettings*, int> Camera_SetSettings;

        public delegate* unmanaged<char*, int, Vector3*, Quaternion*, ulong, ulong> Scene_Instantiate;
        public delegate* unmanaged<char*, int, int> Scene_Load;
        public delegate* unmanaged<char*, int, int> Scene_GetName;
        public delegate* unmanaged<char*, int, int> Scene_GetPath;

        public delegate* unmanaged<ulong, char*, int, int, float, int> Animator_SetParameter;
        public delegate* unmanaged<ulong, char*, int, int, float*, int> Animator_GetParameter;
        public delegate* unmanaged<ulong, char*, int, float, int> Animator_Play;
        public delegate* unmanaged<ulong, NativeAnimatorInfo*, int> Animator_GetInfo;
        public delegate* unmanaged<ulong, char*, int, int> Animator_GetState;

        public delegate* unmanaged<ulong, NativeUIRect*, int> UIRect_Get;
        public delegate* unmanaged<ulong, NativeUIRect*, int> UIRect_Set;
        public delegate* unmanaged<ulong, char*, int, int> UIText_GetText;
        public delegate* unmanaged<ulong, char*, int, int> UIText_SetText;
        public delegate* unmanaged<ulong, NativeUITextStyle*, int> UIText_GetStyle;
        public delegate* unmanaged<ulong, NativeUITextStyle*, int> UIText_SetStyle;
        public delegate* unmanaged<ulong, char*, int, int> UIText_GetFont;
        public delegate* unmanaged<ulong, char*, int, int> UIText_SetFont;
        public delegate* unmanaged<ulong, Vector4*, int> UIImage_GetColor;
        public delegate* unmanaged<ulong, Vector4*, int> UIImage_SetColor;
        public delegate* unmanaged<ulong, char*, int, int> UIImage_GetTexture;
        public delegate* unmanaged<ulong, char*, int, int> UIImage_SetTexture;
        public delegate* unmanaged<int, void> Input_SetCursorLocked;
        public delegate* unmanaged<int> Input_GetCursorState;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeInputAction
    {
        public Vector2 Value;
        public int Held, Pressed, Released, Exists;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeCharacterState
    {
        public int Ground;
        public Vector3 Velocity;
        public Vector3 GroundNormal;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeRaycastHit
    {
        public ulong Entity;
        public Vector3 Point;
        public Vector3 Normal;
        public float Distance;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeAudioSettings
    {
        public float Volume, Pitch;
        public float MinDistance, MaxDistance;
        public int Bus;
        public int Loop, Spatial;
        public int Reserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeCameraSettings
    {
        public int Primary;
        public float FieldOfView;
        public float NearClip, FarClip;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeUIRect
    {
        public Vector2 AnchorMin, AnchorMax, Pivot, Position, Size;
        public int Visible, Reserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeUITextStyle
    {
        public Vector4 Color;
        public float Size, LineSpacing;
        public int Align, VerticalAlign, Wrap, Reserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeAnimatorInfo
    {
        public float StateTime, NormalizedTime, FadeProgress;
        public int InTransition;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct NativeTime
    {
        public float DeltaTime, FixedDeltaTime;
        public double Time;
        public ulong FrameCount, FixedStepCount;
    }

    // A script field's value (ScriptValue). Type: FieldKind. GetField: C++ gives Text room for TextCapacity characters
    [StructLayout(LayoutKind.Sequential)]
    unsafe struct NativeValue
    {
        public int Type;
        public int TextLength;
        public long Integer;
        public double Number;
        public Vector4 Vector;
        public ulong Entity;
        public char* Text;
        public int TextCapacity;
        public int Reserved;
    }

    // A class (Name null) or one of its fields (ScriptFieldReport)
    [StructLayout(LayoutKind.Sequential)]
    unsafe struct NativeFieldReport
    {
        public char* Class;
        public char* Name;
        public char* TypeName;
        public char* EnumNames;
        public long* EnumValues;
        public int ClassLength, NameLength, TypeNameLength, EnumNamesLength;
        public int EnumCount;
        public int Reserved;
        public NativeValue Default;
    }
}
