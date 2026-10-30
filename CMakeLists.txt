cmake_minimum_required(VERSION 3.25)

project(condemned2recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include("${CMAKE_CURRENT_SOURCE_DIR}/generated/rexglue.cmake")

# -------------------------------------------------
# Condemned2Recomp
# -------------------------------------------------
set(CONDEMNED2RECOMP_SOURCES
    src/main.cpp
    src/condemned2_settings.cpp
    src/condemned2_input.cpp
    src/condemned2_graphics.cpp
    src/condemned2_hooks.cpp
    src/condemned2_assets.cpp
    src/condemned2_cheats.cpp
    src/condemned2_multiplayer.cpp
    src/ui/condemned2_launcher.cpp
)

if(WIN32)
    add_executable(condemned2recomp WIN32
        ${CONDEMNED2RECOMP_SOURCES}
    )
else()
    add_executable(condemned2recomp
        ${CONDEMNED2RECOMP_SOURCES}
    )

    target_include_directories(condemned2recomp PRIVATE
        "${REXSDK_DIR}/thirdparty/imgui"
    )
endif()

rexglue_setup_target(condemned2recomp)

# -------------------------------------------------
# Condemned2Recomp Updater
# -------------------------------------------------
add_executable(condemned2recomp_updater
    src/main_updater.cpp
)

target_include_directories(condemned2recomp_updater PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# -------------------------------------------------
# Dependencies
# -------------------------------------------------
function(condemned2_setup_dependencies)
    set(BUILD_SHARED_LIBS OFF)

    # -------------------------------------------------
    # zlib
    # -------------------------------------------------
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(zlib)
    if(NOT TARGET zlibstatic)
        message(FATAL_ERROR "Expected the zlibstatic target from zlib")
    endif()
    target_include_directories(zlibstatic PUBLIC
        "$<BUILD_INTERFACE:${zlib_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${zlib_BINARY_DIR}>"
    )
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    endif()
    set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}")
    set(ZLIB_LIBRARY zlibstatic)

    # -------------------------------------------------
    # curl
    # -------------------------------------------------
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    if(WIN32)
        set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    else()
        set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_Declare(curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_10_1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(curl)
    target_link_libraries(condemned2recomp PRIVATE
        CURL::libcurl
    )

    # -------------------------------------------------
    # libzip
    # -------------------------------------------------
    set(LIBZIP_DO_INSTALL OFF CACHE BOOL "" FORCE)
    set(ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
    set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
    set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(BUILD_REGRESS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_DOC OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG 8e037534ccd62f444abbe4ce265443ee74bbfeb3
    )
    FetchContent_MakeAvailable(libzip)
    target_link_libraries(condemned2recomp PRIVATE
        libzip::zip
    )
    target_link_libraries(condemned2recomp_updater PRIVATE
        libzip::zip
    )

    # -------------------------------------------------
    # PicoSHA2
    # -------------------------------------------------
    FetchContent_Declare(pico
        GIT_REPOSITORY https://github.com/okdshin/PicoSHA2.git
        GIT_TAG 161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29
    )
    FetchContent_MakeAvailable(pico)
    target_include_directories(condemned2recomp PRIVATE
        "${pico_SOURCE_DIR}"
    )
    target_include_directories(condemned2recomp_updater PRIVATE
        "${pico_SOURCE_DIR}"
    )
endfunction()

condemned2_setup_dependencies()

# -------------------------------------------------
# JSON
# -------------------------------------------------
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 55f93686c01528224f448c19128836e7df245f72
)

FetchContent_MakeAvailable(json)
target_link_libraries(condemned2recomp PRIVATE
    nlohmann_json::nlohmann_json
)