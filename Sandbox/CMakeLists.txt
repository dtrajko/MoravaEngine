if(WIN32)

	project(Sandbox LANGUAGES CSharp)

	# Include CMake utilities for CSharp, for WinForm and WPF application support.
	include(CSharpUtilities)

	SET(ExampleApp_SRCS
		../MoravaEngine/src/H2M/ExampleApp/src/BasicController.cs
		../MoravaEngine/src/H2M/ExampleApp/src/MapGenerator.cs
		../MoravaEngine/src/H2M/ExampleApp/src/Noise.cs
		../MoravaEngine/src/H2M/ExampleApp/src/PlayerCube.cs
		../MoravaEngine/src/H2M/ExampleApp/src/RandomColor.cs
		../MoravaEngine/src/H2M/ExampleApp/src/Script.cs
		../MoravaEngine/src/H2M/ExampleApp/src/Sink.cs
	)

	SET(Hazel-ScriptCore_SRCS
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Math/Matrix4.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Math/Noise.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Math/Vector2.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Math/Vector3.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Math/Vector4.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Color.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Material.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Mesh.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/MeshFactory.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Texture2D.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Scene/Component.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Entity.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/Input.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/KeyCodes.cs
		../MoravaEngine/src/H2M/Hazel-ScriptCore/src/Hazel/SpriteRenderer.cs
	)

	add_library(ExampleApp			SHARED "${ExampleApp_SRCS}")
	add_library(Hazel-ScriptCore	SHARED "${Hazel-ScriptCore_SRCS}")

endif()
