#pragma once

#ifndef  NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif // !_WIN32

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"

#ifndef COMMONVALS
#define COMMONVALS
#endif // !COMMONVALS

#include "stb_image.h"
#include "stb_image_write.h"

const float numPI = 3.14159265358979323846f;
const float toRadians = numPI / 180.0f;

const int MAX_POINT_LIGHTS = 4;
const int MAX_SPOT_LIGHTS = 4;
