// The temporal filter for one-channel images (SceneEnvMapVulkan: the ambient occlusion). All of it is in
// Include/TemporalFilter.glslh; this file only names the format.
#type compute
#version 450 core

#define TEMPORAL_FORMAT r8
#include "Include/TemporalFilter.glslh"
