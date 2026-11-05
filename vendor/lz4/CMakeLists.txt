# LZ4 1.10.0 (lib/ of https://github.com/lz4/lz4, unchanged; BSD 2-clause): the compression of the pack file
# (docs/engine/Game_Engine_Runtime_Guide.html, 10.3). lz4hc.c writes packs and lz4.c reads them.
#
# Optimized in every configuration: a Debug build would otherwise decompress several times slower
if(MSVC)
    foreach(flag /RTC1 /Od /Ob0)
        string(REPLACE "${flag}" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    endforeach()
    string(APPEND CMAKE_C_FLAGS_DEBUG " /O2 /Ob2")
else()
    string(REPLACE "-O0" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(APPEND CMAKE_C_FLAGS_DEBUG " -O2")
endif()

add_library(lz4 STATIC lz4.c lz4hc.c)
target_include_directories(lz4 SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(lz4 PRIVATE /w)
else()
    target_compile_options(lz4 PRIVATE -w)
endif()
