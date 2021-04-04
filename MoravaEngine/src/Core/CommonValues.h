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

#include "../../stb_image.h"
#include "../../stb_image_write.h"

const float numPI = 3.14159265358979323846f;
const float toRadians = numPI / 180.0f;

#ifndef M_PI
#define M_PI numPI
#endif

const int MAX_POINT_LIGHTS = 4;
const int MAX_SPOT_LIGHTS = 4;

const int HDRI_GREENWICH_PARK        = 0;
const int HDRI_SAN_GIUSEPPE_BRIDGE   = 1;
const int HDRI_TROPICAL_BEACH        = 2;
const int HDRI_VIGNAIOLI_NIGHT       = 3;
const int HDRI_EARLY_EVE_WARM_SKY    = 4;
const int HDRI_BIRCHWOOD             = 5;
const int HDRI_PINK_SUNRISE          = 6;
const int HDRI_ROOITOU_PARK          = 7;
const int HDRI_VENICE_DAWN           = 8;
const int HDRI_PEPPERMINT_POWERPLANT = 9;

const int ACTION_ADD_MESH = 0;
const int ACTION_ADD_MODEL = 1;

const int PBR_MAP_ENVIRONMENT = 0;
const int PBR_MAP_IRRADIANCE = 1;
const int PBR_MAP_PREFILTER = 2;
