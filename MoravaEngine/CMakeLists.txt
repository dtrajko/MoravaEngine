cmake_minimum_required(VERSION 3.8)

if(WIN32)
	add_compile_definitions(HZ_PLATFORM_WINDOWS)
elseif(UNIX)
	add_compile_definitions(HZ_PLATFORM_LINUX)
endif()

set(ENV{MORAVA_BUILD_TYPE} Debug)
add_compile_definitions(HZ_DEBUG)

set(MSVC_INCREMENTAL_DEFAULT ON)

project("MoravaEngine" VERSION 0.0.1)


get_filename_component(ROOT_PATH_ABS ".." ABSOLUTE)
message("ROOT_PATH_ABS: ${ROOT_PATH_ABS}")


file(GLOB_RECURSE SOURCES ./src/*.cpp ./src/*.h)
source_group(TREE ../src PREFIX src FILES ${SOURCES})

# src/Vendors/ImGui ../vendor/ImGui
file(GLOB_RECURSE SOURCES_IMGUI ../vendor/ImGui/*.cpp)
source_group(TREE ../../vendor/ImGui PREFIX vendor/ImGui FILES ${SOURCES_IMGUI})
set_property(SOURCE ../../vendor/ImGui/main.cpp PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
set_property(SOURCE ../../vendor/ImGui/backends/imgui_impl_dx12.cpp PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
set_property(SOURCE ../../vendor/ImGui/backends/imgui_impl_vulkan.cpp PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
file(GLOB_RECURSE SOURCES_IMGUI_EXAMPLES ../vendor/ImGui/examples/*.cpp)
set_property(SOURCE ${SOURCES_IMGUI_EXAMPLES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")

# Vendors/ImGuizmo ../vendor/cross-platform/ImGuizmo
file(GLOB_RECURSE SOURCES_IMGUIZMO ../vendor/cross-platform/ImGuizmo/*.cpp ../vendor/cross-platform/ImGuizmo/*.h)
source_group(TREE ../../vendor/cross-platform/ImGuizmo PREFIX vendor/ImGuizmo FILES ${SOURCES_IMGUIZMO})
file(GLOB_RECURSE SOURCES_IMGUIZMO_EXAMPLE ../vendor/cross-platform/ImGuizmo/example/*.cpp)
set_property(SOURCE ${SOURCES_IMGUIZMO_EXAMPLE} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")

# Vendors/miniaudio ../vendor/miniaudio/include
file(GLOB_RECURSE SOURCES_MINIAUDIO ../vendor/miniaudio/*.c ../vendor/miniaudio/*.h)
source_group(TREE ../../vendor/miniaudio PREFIX vendor/miniaudio FILES ${SOURCES_MINIAUDIO})
#set_property(SOURCE ../../vendor/miniaudio/include/stb_vorbis.c PROPERTY VS_SETTINGS "ExcludedFromBuild=true")

# Vendors/VulkanMemoryAllocator vendor/VulkanMemoryAllocator
file(GLOB_RECURSE SOURCES_VMA ../vendor/VulkanMemoryAllocator/*.cpp ../vendor/VulkanMemoryAllocator/*.h)
source_group(TREE ../../vendor/VulkanMemoryAllocator PREFIX vendor/VulkanMemoryAllocator FILES ${SOURCES_VMA})

# src/Resources
file(GLOB_RECURSE RESOURCES ./Resources/Shaders/*.glsl)
source_group(TREE ../Resources PREFIX resources FILES ${RESOURCES})
set_property(SOURCE ${RESOURCES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")


file(GLOB_RECURSE BulletDynamics_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletDynamics/*.cpp)
file(GLOB_RECURSE BulletCollision_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletCollision/*.cpp)
file(GLOB_RECURSE LinearMath_SRCS			${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/LinearMath/*.cpp)
file(GLOB_RECURSE box2d_SRCS				${ROOT_PATH_ABS}/vendor/box2d/src/*.cpp)
file(GLOB_RECURSE box2d_SRCS				${ROOT_PATH_ABS}/vendor/box2d/src/*.cpp)
file(GLOB_RECURSE SPIRV-Cross_SRCS			${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
file(GLOB_RECURSE yaml-cpp_SRCS				${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/src/*.cpp)
#	file(GLOB_RECURSE glfw3_SRCS				${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/*.c)
#	file(GLOB_RECURSE zlibstatic_SRCS			${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/*.c)
#	file(GLOB_RECURSE irrXML_SRCS				${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/irrXML/*.cpp)
#	file(GLOB_RECURSE spirv-cross-core_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-cpp_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-glsl_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-hlsl_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-msl_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-reflect_SRCS	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-util_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)


##	if(WIN32)
##		SET(glfw3_SRCS	
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/context.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/init.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/input.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/monitor.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.c		
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/vulkan.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.c		
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_init.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_monitor.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_thread.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_time.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_window.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/window.c
##			)
##	elseif(UNIX)
##		SET(glfw3_SRCS
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/glfw_config.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include/GLFW/glfw3.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include/GLFW/glfw3native.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/internal.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/mappings.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/context.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/init.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/input.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/monitor.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/vulkan.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/window.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_platform.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.h
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.h
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_init.c
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.c
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_monitor.c
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_time.c
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_thread.c
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_window.c
##			#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.c
##			${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.c
##		)
##	endif()

SET(glfw3_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/context.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/init.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/input.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/monitor.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.c		
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/vulkan.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.c		
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_init.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_monitor.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_thread.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_time.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_window.c
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/window.c
)

SET(DirectXTex_SRCS
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/BC.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/BC4BC5.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/BC6HBC7.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexCompress.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexConvert.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexDDS.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexHDR.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexImage.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexMipmaps.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexMisc.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexNormalMaps.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexPMAlpha.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexResize.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexTGA.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexUtil.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexFlipRotate.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexWIC.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/BCDirectCompute.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexCompressGPU.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexD3D11.cpp
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/DirectXTexD3D12.cpp
)

SET(spirv-cross-core_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
)

SET(spirv-cross-cpp_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cpp.cpp
)

SET(spirv-cross-glsl_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
)

SET(spirv-cross-hlsl_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_hlsl.cpp
)

SET(spirv-cross-msl_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_msl.cpp
)

SET(spirv-cross-reflect_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_reflect.cpp
)

SET(spirv-cross-util_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_util.cpp
)

SET(SPIRV-Cross_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_reflect.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_util.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_hlsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cpp.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_msl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/main.cpp
)

##########################################################################################

message("Environment variable 'MORAVA_BUILD_TYPE' = '" $ENV{MORAVA_BUILD_TYPE} "'")

option(USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build) # save executable in root directory

set(CMAKE_GENERATOR_PLATFORM, x64)

unset(CMAKE_IMPORT_LIBRARY_SUFFIX)


add_library(box2d				STATIC "${box2d_SRCS}")
add_library(BulletCollision		STATIC "${BulletCollision_SRCS}")
add_library(BulletDynamics		STATIC "${BulletDynamics_SRCS}")
add_library(LinearMath			STATIC "${LinearMath_SRCS}")
add_library(DirectXTex          STATIC "${DirectXTex_SRCS}")
add_library(glfw3               STATIC "${glfw3_SRCS}")
add_library(SPIRV-Cross			STATIC "${SPIRV-Cross_SRCS}")
add_library(spirv-cross-core	STATIC "${spirv-cross-core_SRCS}")
add_library(spirv-cross-cpp		STATIC "${spirv-cross-cpp_SRCS}")
add_library(spirv-cross-glsl	STATIC "${spirv-cross-glsl_SRCS}")
add_library(spirv-cross-hlsl	STATIC "${spirv-cross-hlsl_SRCS}")
add_library(spirv-cross-msl		STATIC "${spirv-cross-msl_SRCS}")
add_library(spirv-cross-reflect	STATIC "${spirv-cross-reflect_SRCS}")
add_library(spirv-cross-util	STATIC "${spirv-cross-util_SRCS}")
add_library(yaml-cpp			STATIC "${yaml-cpp_SRCS}")
#	add_library(assimp				STATIC "${assimp_SRCS}")
#	add_library(irrXML				STATIC "${irrXML_SRCS}")
#	add_library(zlibstatic			STATIC "${zlibstatic_SRCS}")
#	add_library(shaderc				STATIC "${shaderc_SRCS}")
#	add_library(Hazel-ScriptCore	SHARED "${Hazel-ScriptCore_SRCS}")
#	add_library(ExampleApp			SHARED "${ExampleApp_SRCS}")

#	set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
#	set_property(TARGET ExampleApp PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
#	
#	set(CMAKE_CSharp_FLAGS "/langversion:6")
#	
#	set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_REFERENCES
#		"Microsoft.CSharp"
#		"System"
#		"System.Core"
#		"System.Data"
#		"System.Deployment"
#		"System.Drawing"
#		"System.Net.Http"
#		"System.Xml"
#	)
#	
#	set_property(TARGET ExampleApp PROPERTY VS_DOTNET_REFERENCES
#		"Microsoft.CSharp"
#		"System"
#		"System.Core"
#		"System.Data"
#		"System.Deployment"
#		"System.Drawing"
#		"System.Net.Http"
#		"System.Xml"
#	)


target_include_directories(
	box2d PUBLIC
	${ROOT_PATH_ABS}/vendor/box2d/include
	${ROOT_PATH_ABS}/vendor/box2d/src
)

target_include_directories(
	BulletCollision PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	BulletCollision PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib                 # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletCollision # Linux
)

target_include_directories(
	BulletDynamics PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	BulletDynamics PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib                # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletDynamics # Linux
)

target_include_directories(
	LinearMath PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	LinearMath PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib            # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/LinearMath # Linux
)

target_include_directories(
	glfw3 PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
)

target_link_directories(
	glfw3 PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
)

target_link_directories(
	SPIRV-Cross PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross
)

target_include_directories(
	yaml-cpp PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/include
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/src
)


#	target_include_directories(
#		zlibstatic PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib
#	)

#	target_include_directories(
#		assimp PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/include
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/irrXML
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/unzip
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/openddlparser/include
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/rapidjson/include
#	)

#	target_link_directories(
#		assimp PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/lib/$ENV{MORAVA_BUILD_TYPE} # Win32
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/bin                         # Linux
#	)

#	target_link_directories(
#		zlibstatic PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/$ENV{MORAVA_BUILD_TYPE}
#	)

#	target_include_directories(
#		shaderc PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/include
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/include
#	)

#	target_link_directories(
#		shaderc PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE}
#	)

#	target_include_directories(
#		Hazel-ScriptCore PUBLIC
#		../MoravaEngine/src/H2M/Hazel-ScriptCore
#	)
#	
#	target_include_directories(
#		ExampleApp PUBLIC
#		../MoravaEngine/src/H2M/ExampleApp
#	)
#	
#	target_link_directories(
#		Hazel-ScriptCore PUBLIC
#		../MoravaEngine/src/H2M/Hazel-ScriptCore
#	)
#	
#	target_link_directories(
#		ExampleApp PUBLIC
#		../MoravaEngine/src/H2M/ExampleApp
#	)

################################################################################

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_IMGUI} ${SOURCES_IMGUIZMO} ${RESOURCES} ${SOURCES_MINIAUDIO} ${SOURCES_VMA})

add_dependencies(SPIRV-Cross spirv-cross-core)
add_dependencies(SPIRV-Cross spirv-cross-cpp)
add_dependencies(SPIRV-Cross spirv-cross-glsl)
add_dependencies(SPIRV-Cross spirv-cross-hlsl)
add_dependencies(SPIRV-Cross spirv-cross-msl)
add_dependencies(SPIRV-Cross spirv-cross-reflect)
add_dependencies(SPIRV-Cross spirv-cross-util)
add_dependencies("${PROJECT_NAME}" box2d)
add_dependencies("${PROJECT_NAME}" BulletCollision)
add_dependencies("${PROJECT_NAME}" BulletDynamics)
add_dependencies("${PROJECT_NAME}" LinearMath)
#	add_dependencies("${PROJECT_NAME}" DirectXTex)
#	add_dependencies("${PROJECT_NAME}" glfw3)
add_dependencies("${PROJECT_NAME}" SPIRV-Cross)
add_dependencies("${PROJECT_NAME}" yaml-cpp)
#	add_dependencies(assimp irrXML)
#	add_dependencies(assimp zlibstatic)
#	add_dependencies("${PROJECT_NAME}" assimp)
#	add_dependencies("${PROJECT_NAME}" shaderc)
#	add_dependencies("${PROJECT_NAME}" Hazel-ScriptCore)
#	add_dependencies("${PROJECT_NAME}" ExampleApp)

if(WIN32)
	set_property(TARGET box2d					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET BulletCollision			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET BulletDynamics			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET LinearMath				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET DirectXTex              PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET glfw3					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET SPIRV-Cross				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-core		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-cpp			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-glsl		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-hlsl		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-msl			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-reflect		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET spirv-cross-util		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_property(TARGET yaml-cpp				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	#	set_property(TARGET assimp				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	#	set_property(TARGET irrXML				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	#	set_property(TARGET shaderc				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	#	set_property(TARGET zlibstatic			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	set_property(TARGET	glfw3					PROPERTY USE_MSVC_RUNTIME_LIBRARY_DLL "OFF")

	set_property(TARGET	"${PROJECT_NAME}"		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(BulletDynamics	PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(BulletCollision   PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(LinearMath		PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(glfw3				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(assimp			PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)

set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(BulletDynamics	PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(BulletCollision   PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(LinearMath		PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(glfw3				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
#	set_target_properties(assimp			PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)

target_compile_definitions(glfw3 PRIVATE _GLFW_USE_CONFIG_H)
#	target_compile_definitions(glfw3 PRIVATE _GLFW_WIN32)

target_compile_features("${PROJECT_NAME}"	PRIVATE cxx_std_17)
#	target_compile_features(assimp				PRIVATE cxx_std_17)

target_compile_features("${PROJECT_NAME}"	PRIVATE cxx_std_17)

# -- BEGIN Main project

target_include_directories(
	"${PROJECT_NAME}" PUBLIC
	${ROOT_PATH_ABS}/${PROJECT_NAME}/src
	${ROOT_PATH_ABS}/vendor
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/include
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
	${ROOT_PATH_ABS}/vendor/cross-platform/ImGuizmo
	${ROOT_PATH_ABS}/vendor/GLEW/include
	${ROOT_PATH_ABS}/vendor/ImGui
	${ROOT_PATH_ABS}/vendor/GLM
	${ROOT_PATH_ABS}/vendor/tinyobjloader
	${ROOT_PATH_ABS}/vendor/spdlog/include
    ${ROOT_PATH_ABS}/vendor/entt/include
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/include
	${ROOT_PATH_ABS}/vendor/mono/include
	${ROOT_PATH_ABS}/vendor/box2d/include
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/include
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/include
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross
	${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Include
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex
	${ROOT_PATH_ABS}/vendor/Optick/src
	${ROOT_PATH_ABS}/vendor/miniaudio/include
	${ROOT_PATH_ABS}/vendor/farbot/include
	${ROOT_PATH_ABS}/vendor/PhysX/include
	${ROOT_PATH_ABS}/vendor/PhysX/include/PhysX
	${ROOT_PATH_ABS}/vendor/NvidiaAftermath/include
)

SET(LINK_DIRS_CROSS_PLATFORM
	${ROOT_PATH_ABS}/${PROJECT_NAME}/build/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/GLEW/lib
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Lib
	${ROOT_PATH_ABS}/vendor/DirectXTex/bin/CMake/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/mono/lib/$ENV{MORAVA_BUILD_TYPE}
)

if(WIN32)
	target_link_directories(
		${PROJECT_NAME} PUBLIC
		${LINK_DIRS_CROSS_PLATFORM}
		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib                                     # Win32
		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/lib/$ENV{MORAVA_BUILD_TYPE}              # Win32
		${ROOT_PATH_ABS}/vendor/box2d/bin/$ENV{MORAVA_BUILD_TYPE}                              # Win32
		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}      # Win32
		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE} # Win32
	)
endif()

if(UNIX)
	target_link_directories(
		${PROJECT_NAME} PUBLIC
		${LINK_DIRS_CROSS_PLATFORM}
		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletDynamics  # Linux
		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletCollision # Linux
		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/LinearMath      # Linux
		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/bin                  # Linux
		${ROOT_PATH_ABS}/vendor/box2d/build/bin                            # Linux
		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc          # Linux
		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util     # Linux
	)
endif()

if(WIN32)
	target_link_libraries(
		"${PROJECT_NAME}"
		opengl32.lib
		glew32.lib
		glfw3.lib
		BulletDynamics.lib
		BulletCollision.lib
		LinearMath.lib
	#	yaml-cppd.lib
		yaml-cpp.lib
		mono-2.0-sgen.lib
		box2d.lib
		shaderc_combined.lib
		vulkan-1.lib
		d3d11.lib
		d3dcompiler.lib
		DirectXTex.lib
		spirv-cross-core.lib
		spirv-cross-cpp.lib
		spirv-cross-glsl.lib
		spirv-cross-hlsl.lib
		spirv-cross-msl.lib
		spirv-cross-reflect.lib
		spirv-cross-util.lib
	)

	if(WIN32)
		target_link_libraries("${PROJECT_NAME}" debug assimp-vc142-mtd.lib)
		target_link_libraries("${PROJECT_NAME}" optimized assimp-vc142-mt.lib)
	endif()

endif()

if(UNIX)
	target_link_libraries(
		"${PROJECT_NAME}"
		libGLEW.so # -lGLEW
		libGLU.so  # -lGLU
		libGL.so   # -lGL
		# libglfw3.a
		libassimp.so
		libBulletDynamics.a
		libBulletCollision.a
		libLinearMath.a
		libbox2d.a
		libshaderc.a
		libshaderc_util.a
	)
	target_link_libraries("${PROJECT_NAME}" libglfw3.a dl pthread)
endif()


#	if ($ENV{MORAVA_BUILD_TYPE} == 'Debug')
#		target_link_libraries(
#			"${PROJECT_NAME}"
#			spirv-cross-cored.lib		# Debug
#			spirv-cross-cppd.lib		# Debug
#			spirv-cross-glsld.lib		# Debug
#			spirv-cross-hlsld.lib		# Debug
#			spirv-cross-msld.lib		# Debug
#			spirv-cross-reflectd.lib	# Debug
#			spirv-cross-utild.lib		# Debug
#		)
#	elseif($ENV{MORAVA_BUILD_TYPE} == 'Release')
#		target_link_libraries(
#			"${PROJECT_NAME}"
#			spirv-cross-core.lib		# Release
#			spirv-cross-cpp.lib			# Release
#			spirv-cross-glsl.lib		# Release
#			spirv-cross-hlsl.lib		# Release
#			spirv-cross-msl.lib			# Release
#			spirv-cross-reflect.lib		# Release
#			spirv-cross-util.lib		# Release
#		)
#	endif()

#	file(COPY ./assimp-vc142-mt.dll		DESTINATION ../build)
#	file(COPY ./glew32.dll				DESTINATION ../build)
#	file(COPY ./Models					DESTINATION ../build)
#	file(COPY ./Shaders					DESTINATION ../build)
#	file(COPY ./Textures				DESTINATION ../build)

############################################################################

# Build assimp with CMake
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/assimp)
execute_process(COMMAND cmake --build . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/assimp)

# Build GLFW with CMake
# file(COPY ${ROOT_PATH_ABS}/${PROJECT_NAME}/CMake/GLFW/CMakeLists.txt DESTINATION ${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src)
# execute_process(COMMAND cmake -S . -B . -DBUILD_SHARED_LIBS=OFF . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/glfw)
# execute_process(COMMAND cmake --build . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/glfw)

# Build DirectXTex
# file(COPY ${ROOT_PATH_ABS}/${PROJECT_NAME}/CMake/DirectXTex/CMakeLists.txt DESTINATION ${ROOT_PATH_ABS}/vendor/DirectXTex)
# execute_process(COMMAND cmake -DBUILD_SHARED_LIBS=OFF . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/DirectXTex)
# execute_process(COMMAND cmake --build . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/DirectXTex)

# Build shaderc - use python to run git-sync-deps
execute_process(COMMAND python git-sync-deps WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/utils)
# Build shaderc with CMake
execute_process(COMMAND cmake -DSHADERC_SKIP_EXAMPLES=ON -DSHADERC_SKIP_TESTS=ON . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc)
execute_process(COMMAND cmake --build . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc)

# Build yaml-cpp
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp)
execute_process(COMMAND cmake --build . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp)

# Delete directory bullet3/examples/ThirdPartyLibs/imgui to avoid ImGui conflicts in compilation
file(REMOVE_RECURSE ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/imgui)


# dirty fix for missing C:/Windows/System32/vulkan-1.dll until better solution
# file(COPY ./vulkan-1.dll DESTINATION C:/Windows/System32)


set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

set_property(TARGET "${PROJECT_NAME}" PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "..")
