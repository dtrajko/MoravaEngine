cmake_minimum_required(VERSION 3.16)

set(MSVC_INCREMENTAL_DEFAULT ON)


# set(BULLET_VERSION, 2.85)
# project("BulletCollision" VERSION "${BULLET_VERSION}")
# project("BulletDynamics"  VERSION "${BULLET_VERSION}")
# project("LinearMath"      VERSION "${BULLET_VERSION}")


project("MoravaEngine" VERSION 0.0.1)

option(USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF)

file(GLOB SOURCES
	./*.h
	./*.cpp
	PerlinNoise/PerlinNoise.hpp
	LearnOpenGL/*.h
	LearnOpenGL/*.cpp
	SongHo/*.h
	SongHo/*.cpp
	../vendor/ImGui/imconfig.h
	../vendor/ImGui/imgui.cpp
	../vendor/ImGui/imgui.h
	../vendor/ImGui/imgui_demo.cpp
	../vendor/ImGui/imgui_draw.cpp
	../vendor/ImGui/imgui_impl_dx11.cpp
	../vendor/ImGui/imgui_impl_dx11.h
	../vendor/ImGui/imgui_impl_glfw.h
	../vendor/ImGui/imgui_impl_opengl3.h
	../vendor/ImGui/imgui_impl_win32.cpp
	../vendor/ImGui/imgui_impl_win32.h
	../vendor/ImGui/imgui_internal.h
	../vendor/ImGui/imgui_widgets.cpp
	../vendor/ImGui/imstb_rectpack.h
	../vendor/ImGui/imstb_textedit.h
	../vendor/ImGui/imstb_truetype.h
)

add_executable("MoravaEngine" "${SOURCES}")

set(CMAKE_GENERATOR_PLATFORM, x64)

# if(MSVC)
# 	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG:incremental")
# endif()

unset(CMAKE_IMPORT_LIBRARY_SUFFIX)

include_directories(
	../vendor/GLFW/include
	../vendor/GLEW/include
	../vendor/ASSIMP/include
	../vendor/ImGui
	../vendor/GLM
	../vendor/bullet3/src
	../vendor/tinyobjloader
	../vendor/spdlog/include
)

target_link_directories(
	"MoravaEngine" PUBLIC
	../vendor/GLFW/lib-vc2019
	../vendor/ASSIMP/lib
	../vendor/GLEW/lib
	../vendor/bullet3/lib
)

target_link_libraries(
	"MoravaEngine"
	opengl32.lib
	glew32.lib
	glfw3.lib
	assimp-vc140-mt.lib
)

file(COPY ./assimp-vc140-mt.dll DESTINATION ../build)
file(COPY ./glew32.dll DESTINATION ../build)
file(COPY ./Models DESTINATION ../build)
file(COPY ./Shaders DESTINATION ../build)
file(COPY ./Textures DESTINATION ../build)


# set(BULLET_ROOT, ../vendor/bullet3)
# set(BULLET_DYNAMICS_LIBRARY, ../vendor/bullet3/src/BulletDynamics)
# set(BULLET_COLLISION_LIBRARY, ../vendor/bullet3/src/BulletCollision)
# set(BULLET_MATH_LIBRARY, ../vendor/bullet3/src/LinearMath)
# set(BULLET_SOFTBODY_LIBRARY, ../vendor/bullet3/src/BulletSoftBody)
# set(BULLET_INCLUDE_DIR, ../vendor/bullet3/src)
# find_package(Bullet REQUIRED)

# set(BULLET_VERSION, 2.85)
# include(../vendor/bullet3/src/CMakeLists.txt)
# include(../vendor/bullet3/src/BulletCollision/CMakeLists.txt)
# include(../vendor/bullet3/src/BulletDynamics/CMakeLists.txt)
# include(../vendor/bullet3/src/LinearMath/CMakeLists.txt)


# copy files and directories to build folder
# cp MoravaEngine/assimp-vc140-mt.dll build/assimp-vc140-mt.dll
# cp MoravaEngine/glew32.dll build/glew32.dll
# cp MoravaEngine/Models build/Models
# cp MoravaEngine/glew32.dll build/Shaders
# cp MoravaEngine/Textures build/Textures
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++latest")

# Add Bullet projects
# BulletCollision  vendor/bullet3/src/BulletCollision
# BulletDynamic    vendor/bullet3/src/BulletDynamics
# LinearMath       vendor/bullet3/src/LinearMath
