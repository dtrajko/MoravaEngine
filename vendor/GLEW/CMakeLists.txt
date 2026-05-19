cmake_minimum_required(VERSION 3.15)

project(glew)

add_library(glew STATIC
    src/glew.c
)

target_include_directories(glew PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(glew PUBLIC
    GLEW_STATIC
    GLEW_NO_GLU
)

if(WIN32)
    target_link_libraries(glew PUBLIC opengl32)
endif()
