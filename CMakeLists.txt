# condemned2recomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(condemned2recomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# CURL Dependency (Used for downloader)
# -------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_10_1
)
set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_WOLFSSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_MBEDTLS OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(curl)
# -------------------------------------------------

# Sources
set(CONDEMNED2RECOMP_SOURCES
    src/main.cpp
    src/condemned2_settings.cpp
    src/condemned2_input.cpp
    src/condemned2_graphics.cpp
    src/condemned2_hooks.cpp
)

if(WIN32)
    add_executable(condemned2recomp WIN32 ${CONDEMNED2RECOMP_SOURCES})
else()
    add_executable(condemned2recomp ${CONDEMNED2RECOMP_SOURCES})
    target_include_directories(condemned2recomp PRIVATE "${REXSDK_DIR}/thirdparty/imgui" )
endif()

rexglue_setup_target(condemned2recomp)

target_link_libraries(condemned2recomp PRIVATE CURL::libcurl)