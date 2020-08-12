cmake_minimum_required(VERSION 3.1)
project(MoravaEngine)

include_directories(
	"${CMAKE_SOURCE_DIR}/MoravaEngine"
	"${CMAKE_SOURCE_DIR}/vendor/GLFW/include"
	"${CMAKE_SOURCE_DIR}/vendor/GLEW/include"
	"${CMAKE_SOURCE_DIR}/vendor/ASSIMP/include"
	"${CMAKE_SOURCE_DIR}/vendor/GLM"
)

add_subdirectory(vendor/GLM/glm)

add_executable(
	MoravaEngine
	MoravaEngine/main.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++latest")
endif()



function(fizzbuzz n)
	foreach(i RANGE 1 ${n})
		set(result "")
		math(EXPR fizz "${i} % 3")
		math(EXPR buzz "${i} % 5")
		if (NOT fizz)
			string(APPEND result "fizz")
		endif()
		if (NOT buzz)
			string(APPEND result "buzz")
		endif()
		if (NOT result)
			set(result "${i}")
		endif()
		message("${result}")
	endforeach()
endfunction()

fizzbuzz(20)
