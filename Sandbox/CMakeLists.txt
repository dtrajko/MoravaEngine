if(WIN32)

	project(Sandbox LANGUAGES CSharp)

	# Include CMake utilities for CSharp, for WinForm and WPF application support.
	include(CSharpUtilities)

	SET(ExampleApp_SRCS
		../src/H2M/ExampleApp/src/BasicController.cs
		../src/H2M/ExampleApp/src/MapGenerator.cs
		../src/H2M/ExampleApp/src/Noise.cs
		../src/H2M/ExampleApp/src/PlayerCube.cs
		../src/H2M/ExampleApp/src/RandomColor.cs
		../src/H2M/ExampleApp/src/Script.cs
		../src/H2M/ExampleApp/src/Sink.cs
	)

	SET(Hazel-ScriptCore_SRCS
		../src/H2M/Hazel-ScriptCore/src/Hazel/Math/Matrix4.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Math/Noise.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Math/Vector2.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Math/Vector3.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Math/Vector4.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Color.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Material.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Mesh.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/MeshFactory.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Renderer/Texture2D.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Scene/Component.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Entity.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/Input.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/KeyCodes.cs
		../src/H2M/Hazel-ScriptCore/src/Hazel/SpriteRenderer.cs
	)

	add_library(ExampleApp			SHARED "${ExampleApp_SRCS}")
	add_library(Hazel-ScriptCore	SHARED "${Hazel-ScriptCore_SRCS}")

endif()
