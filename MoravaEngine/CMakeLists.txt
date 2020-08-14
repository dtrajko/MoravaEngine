cmake_minimum_required(VERSION 3.16)

set(MSVC_INCREMENTAL_DEFAULT ON)

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
	../vendor/ImGui/*.h
	../vendor/ImGui/*.cpp
)

add_executable("MoravaEngine" "${SOURCES}")

set(CMAKE_GENERATOR_PLATFORM, x64)

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG:incremental")
endif()

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

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++latest")
