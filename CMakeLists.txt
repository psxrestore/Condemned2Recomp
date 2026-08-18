# condemned2recomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(condemned2recomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(CONDEMNED2RECOMP_SOURCES
    src/main.cpp
    src/condemned2_hooks.cpp
    src/condemned2_settings.cpp
)

if(WIN32)
    add_executable(condemned2recomp WIN32 ${CONDEMNED2RECOMP_SOURCES})
else()
    add_executable(condemned2recomp ${CONDEMNED2RECOMP_SOURCES})
    target_include_directories(condemned2recomp PRIVATE "${REXSDK_DIR}/thirdparty/imgui" )
endif()

rexglue_setup_target(condemned2recomp)
