# condemned2recomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(condemned2recomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the bundled SDK submodule unless REXSDK_DIR / an installed SDK is provided.
if(NOT REXSDK_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk/CMakeLists.txt")
    set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH
        "Path to rexglue-sdk source tree" FORCE)
endif()

# Windows checkouts without symlink support turn libmspack's symlinked sources
# into small text files containing the link target. Replace them with copies.
if(WIN32 AND REXSDK_DIR)
    file(GLOB _mspack_files "${REXSDK_DIR}/thirdparty/libmspack/cabextract/mspack/*")
    foreach(_f IN LISTS _mspack_files)
        if(IS_DIRECTORY "${_f}")
            continue()
        endif()
        file(SIZE "${_f}" _size)
        if(_size LESS 256)
            file(READ "${_f}" _content)
            string(STRIP "${_content}" _target)
            get_filename_component(_dir "${_f}" DIRECTORY)
            if(_target MATCHES "^\\.\\./.*mspack/" AND EXISTS "${_dir}/${_target}")
                message(STATUS "Replacing symlink stub ${_f}")
                file(COPY_FILE "${_dir}/${_target}" "${_f}" ONLY_IF_DIFFERENT)
            endif()
        endif()
    endforeach()
endif()

include(generated/rexglue.cmake)

# -------------------------------------------------
# Dependencies
# -------------------------------------------------
include(FetchContent)
set(_prev_shared ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
else()
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_10_1
    GIT_SHALLOW TRUE)
FetchContent_Declare(pico
    GIT_REPOSITORY https://github.com/okdshin/PicoSHA2
    GIT_TAG 161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(curl pico)
set(BUILD_SHARED_LIBS ${_prev_shared} CACHE BOOL "" FORCE)
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
endif()

# Building the SDK from source doesn't export its bundled ImGui headers.
if(REXSDK_DIR)
    target_include_directories(condemned2recomp PRIVATE "${REXSDK_DIR}/thirdparty/imgui")
endif()

rexglue_setup_target(condemned2recomp GPU_PLUGINS xenos)

target_link_libraries(condemned2recomp PRIVATE CURL::libcurl)
target_include_directories(condemned2recomp PRIVATE ${pico_SOURCE_DIR})