/*
 * miniaudio's implementation (docs/engine/Game_Engine_Runtime_Guide.html, 5.1), compiled as C into the miniaudio library
 * of CMakeLists.txt, not into the engine's sources (which are C++). With stb_vorbis for Ogg Vorbis, the way miniaudio's
 * documentation shows: its declarations first, so miniaudio's decoder sees them, then its implementation.
 */

#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
