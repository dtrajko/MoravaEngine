// The temporal filter for four-channel float images (SceneEnvMapVulkan: the ray-traced reflections). All of it is in
// Include/TemporalFilter.glslh; this file only names the format.
#type compute
#version 450 core

#define TEMPORAL_FORMAT rgba16f
#include "Include/TemporalFilter.glslh"
