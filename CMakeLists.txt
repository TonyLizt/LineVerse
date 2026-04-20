cmake_minimum_required(VERSION 3.21)

project(LineVerse VERSION 0.1.0 LANGUAGES CXX)

# ==============================
# Global project configuration
# ==============================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Build output layout
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ==============================
# Project options
# ==============================
option(LV_BUILD_HANDLE_GAME "Build HandleGame module" ON)
option(LV_BUILD_IDIOM_CHAIN_GAME "Build IdiomChainGame module" ON)
option(LV_BUILD_POETRY_REBUILD_GAME "Build PoetryRebuildGame module" ON)
option(LV_BUILD_VERSE_UNFOLD_GAME "Build VerseUnfoldGame module" ON)

option(LV_ENABLE_SDL_IMAGE "Link SDL2_image when available" ON)
option(LV_ENABLE_SDL_TTF "Link SDL2_ttf when available" ON)
option(LV_ENABLE_SDL_MIXER "Link SDL2_mixer when available" ON)

# ==============================
# Third-party library locations
# Expected layout under project root:
#   libs/
#     asio-1.36.0/
#     SDL2-2.26.0-allinone/
# ==============================
set(LV_LIBS_DIR "${PROJECT_SOURCE_DIR}/libs")
set(LV_ASIO_ROOT "${LV_LIBS_DIR}/asio-1.36.0" CACHE PATH "Path to standalone Asio root")
set(LV_SDL2_ROOT "${LV_LIBS_DIR}/SDL2-2.26.0-allinone" CACHE PATH "Path to SDL2 all-in-one package root")

# ==============================
# Compiler warnings / common flags
# ==============================
add_library(LineVerseOptions INTERFACE)
add_library(LineVerse::Options ALIAS LineVerseOptions)

target_compile_features(LineVerseOptions INTERFACE cxx_std_17)

target_compile_definitions(LineVerseOptions INTERFACE
    ASIO_STANDALONE
    ASIO_NO_DEPRECATED
    SDL_MAIN_HANDLED
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
    $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0A00>
)

if(MSVC)
    target_compile_options(LineVerseOptions INTERFACE /W4 /permissive-)
else()
    target_compile_options(LineVerseOptions INTERFACE -Wall -Wextra -Wpedantic)
endif()

# ==============================
# Asio (header-only)
# ==============================
if(NOT EXISTS "${LV_ASIO_ROOT}/include/asio.hpp")
    message(FATAL_ERROR
        "Asio not found. Expected: ${LV_ASIO_ROOT}/include/asio.hpp\n"
        "Please extract asio-1.36.0 into LineVerse/libs/."
    )
endif()

add_library(LineVerseAsio INTERFACE)
add_library(LineVerse::Asio ALIAS LineVerseAsio)

target_include_directories(LineVerseAsio INTERFACE
    "${LV_ASIO_ROOT}/include"
)

target_link_libraries(LineVerseAsio INTERFACE LineVerse::Options)

if(WIN32)
    target_link_libraries(LineVerseAsio INTERFACE ws2_32 mswsock)
endif()

# ==============================
# SDL2
# 手动接入 SDL2 import library，避免使用官方 MinGW 配置文件中的
# SDL2main / -mwindows，保证当前阶段可作为控制台程序运行。
# ==============================
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LV_SDL2_ARCH_DIR "x86_64-w64-mingw32")
    else()
        set(LV_SDL2_ARCH_DIR "i686-w64-mingw32")
    endif()

    set(LV_SDL2_PREFIX "${LV_SDL2_ROOT}/${LV_SDL2_ARCH_DIR}")
    set(LV_SDL2_INCLUDE_DIR "${LV_SDL2_PREFIX}/include/SDL2")
    set(LV_SDL2_LIB_DIR "${LV_SDL2_PREFIX}/lib")
    set(LV_SDL2_BIN_DIR "${LV_SDL2_PREFIX}/bin")

    if(NOT EXISTS "${LV_SDL2_INCLUDE_DIR}/SDL.h")
        message(FATAL_ERROR
            "SDL2 headers not found. Expected: ${LV_SDL2_INCLUDE_DIR}/SDL.h\n"
            "Please check SDL2-2.26.0-allinone extraction."
        )
    endif()

    if(NOT EXISTS "${LV_SDL2_LIB_DIR}/libSDL2.dll.a")
        message(FATAL_ERROR
            "SDL2 import library not found. Expected: ${LV_SDL2_LIB_DIR}/libSDL2.dll.a"
        )
    endif()

    set(LV_SDL2_IMAGE_AVAILABLE OFF)
    set(LV_SDL2_TTF_AVAILABLE OFF)
    set(LV_SDL2_MIXER_AVAILABLE OFF)

    if(LV_ENABLE_SDL_IMAGE AND EXISTS "${LV_SDL2_LIB_DIR}/libSDL2_image.dll.a")
        set(LV_SDL2_IMAGE_AVAILABLE ON)
    endif()

    if(LV_ENABLE_SDL_TTF AND EXISTS "${LV_SDL2_LIB_DIR}/libSDL2_ttf.dll.a")
        set(LV_SDL2_TTF_AVAILABLE ON)
    endif()

    if(LV_ENABLE_SDL_MIXER AND EXISTS "${LV_SDL2_LIB_DIR}/libSDL2_mixer.dll.a")
        set(LV_SDL2_MIXER_AVAILABLE ON)
    endif()
else()
    message(FATAL_ERROR "This CMake configuration currently targets Windows + MinGW SDL2 all-in-one layout.")
endif()

add_library(LineVerseSDL INTERFACE)
add_library(LineVerse::SDL ALIAS LineVerseSDL)

target_link_libraries(LineVerseSDL INTERFACE
    LineVerse::Options
)

target_include_directories(LineVerseSDL INTERFACE
    "${LV_SDL2_INCLUDE_DIR}"
)

# 只链接 SDL2 本体，不链接 SDL2main，也不引入 -mwindows
target_link_libraries(LineVerseSDL INTERFACE
    "${LV_SDL2_LIB_DIR}/libSDL2.dll.a"
)

if(LV_SDL2_IMAGE_AVAILABLE)
    target_link_libraries(LineVerseSDL INTERFACE
        "${LV_SDL2_LIB_DIR}/libSDL2_image.dll.a"
    )
endif()

if(LV_SDL2_TTF_AVAILABLE)
    target_link_libraries(LineVerseSDL INTERFACE
        "${LV_SDL2_LIB_DIR}/libSDL2_ttf.dll.a"
    )
endif()

if(LV_SDL2_MIXER_AVAILABLE)
    target_link_libraries(LineVerseSDL INTERFACE
        "${LV_SDL2_LIB_DIR}/libSDL2_mixer.dll.a"
    )
endif()

# Combined dependency target for all modules/executables.
add_library(LineVerseDeps INTERFACE)
add_library(LineVerse::Deps ALIAS LineVerseDeps)

target_link_libraries(LineVerseDeps INTERFACE
    LineVerse::Options
    LineVerse::Asio
    LineVerse::SDL
)

# ==============================
# Helper: register a module as STATIC library if it has sources,
# otherwise as INTERFACE placeholder to keep project structure stable.
# ==============================
function(lv_add_module TARGET_NAME MODULE_DIR ENABLED)
    if(NOT ${ENABLED})
        return()
    endif()

    set(MODULE_ROOT "${PROJECT_SOURCE_DIR}/src/${MODULE_DIR}")

    if(NOT EXISTS "${MODULE_ROOT}")
        message(STATUS "Module directory not found: ${MODULE_ROOT}. Creating placeholder target ${TARGET_NAME}.")
        add_library(${TARGET_NAME} INTERFACE)
        target_link_libraries(${TARGET_NAME} INTERFACE LineVerse::Deps)
        return()
    endif()

    file(GLOB_RECURSE MODULE_HEADERS CONFIGURE_DEPENDS
        "${MODULE_ROOT}/*.h"
        "${MODULE_ROOT}/*.hpp"
    )
    file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
        "${MODULE_ROOT}/*.c"
        "${MODULE_ROOT}/*.cc"
        "${MODULE_ROOT}/*.cpp"
    )

    if(MODULE_SOURCES)
        add_library(${TARGET_NAME} STATIC ${MODULE_SOURCES} ${MODULE_HEADERS})
        target_link_libraries(${TARGET_NAME} PUBLIC LineVerse::Deps)

        if("${TARGET_NAME}" STREQUAL "VerseUnfoldGameModule")
            target_include_directories(${TARGET_NAME} PUBLIC
                "${PROJECT_SOURCE_DIR}/src"
                "${MODULE_ROOT}"
                "${CMAKE_SOURCE_DIR}/libs"
                "${CMAKE_SOURCE_DIR}/libs/asio-1.36.0/include"
            )
        else()
            target_include_directories(${TARGET_NAME} PUBLIC
                "${PROJECT_SOURCE_DIR}/src"
                "${MODULE_ROOT}"
            )
        endif()

        source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${MODULE_SOURCES} ${MODULE_HEADERS})
        set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "modules")
    else()
        add_library(${TARGET_NAME} INTERFACE)
        target_link_libraries(${TARGET_NAME} INTERFACE LineVerse::Deps)

        if("${TARGET_NAME}" STREQUAL "VerseUnfoldGameModule")
            target_include_directories(${TARGET_NAME} INTERFACE
                "${PROJECT_SOURCE_DIR}/src"
                "${MODULE_ROOT}"
                "${CMAKE_SOURCE_DIR}/libs"
                "${CMAKE_SOURCE_DIR}/libs/asio-1.36.0/include"
            )
        else()
            target_include_directories(${TARGET_NAME} INTERFACE
                "${PROJECT_SOURCE_DIR}/src"
                "${MODULE_ROOT}"
            )
        endif()

        message(STATUS "Module ${MODULE_DIR} currently has no source files. Placeholder target created.")
    endif()
endfunction()

# ==============================
# Register module targets
# ==============================
lv_add_module(HandleGameModule         "HandleGame"         LV_BUILD_HANDLE_GAME)
lv_add_module(IdiomChainGameModule     "IdiomChainGame"     LV_BUILD_IDIOM_CHAIN_GAME)
lv_add_module(PoetryRebuildGameModule  "PoetryRebuildGame"  LV_BUILD_POETRY_REBUILD_GAME)
lv_add_module(VerseUnfoldGameModule    "VerseUnfoldGame"    LV_BUILD_VERSE_UNFOLD_GAME)

# ==============================
# Main executable
# ==============================
# ==============================
# Main executable
# ==============================
set(LV_MAIN_SOURCES)
if(EXISTS "${PROJECT_SOURCE_DIR}/src/main.cpp")
    list(APPEND LV_MAIN_SOURCES "${PROJECT_SOURCE_DIR}/src/main.cpp")
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/src/main.h")
    list(APPEND LV_MAIN_SOURCES "${PROJECT_SOURCE_DIR}/src/main.h")
endif()

# ui
if(EXISTS "${PROJECT_SOURCE_DIR}/src/ui/HomepageScreen.cpp")
    list(APPEND LV_MAIN_SOURCES "${PROJECT_SOURCE_DIR}/src/ui/HomepageScreen.cpp")
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/src/ui/HomepageScreen.h")
    list(APPEND LV_MAIN_SOURCES "${PROJECT_SOURCE_DIR}/src/ui/HomepageScreen.h")
endif()

if(NOT LV_MAIN_SOURCES)
    message(FATAL_ERROR "src/main.cpp not found. Please create the main entry before building.")
endif()

add_executable(LineVerse ${LV_MAIN_SOURCES})
add_executable(LineVerse::App ALIAS LineVerse)

source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${LV_MAIN_SOURCES})

target_include_directories(LineVerse PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/libs"
)

target_link_libraries(LineVerse PRIVATE
    LineVerse::Deps
    HandleGameModule
    IdiomChainGameModule
    PoetryRebuildGameModule
    VerseUnfoldGameModule
)

set_target_properties(LineVerse PROPERTIES
    OUTPUT_NAME "LineVerse"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:LineVerse>"
)

# ==============================
# Runtime asset/data copying
# ==============================
set(LV_ASSETS_DIR "${PROJECT_SOURCE_DIR}/assets")
set(LV_DATA_DIR   "${PROJECT_SOURCE_DIR}/data")

if(EXISTS "${LV_ASSETS_DIR}")
    add_custom_command(TARGET LineVerse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${LV_ASSETS_DIR}"
                "$<TARGET_FILE_DIR:LineVerse>/assets"
        COMMENT "Copying assets directory to runtime output"
    )
endif()

if(EXISTS "${LV_DATA_DIR}")
    add_custom_command(TARGET LineVerse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${LV_DATA_DIR}"
                "$<TARGET_FILE_DIR:LineVerse>/data"
        COMMENT "Copying data directory to runtime output"
    )
endif()

# ==============================
# Copy SDL runtime DLLs
# ==============================
if(WIN32 AND EXISTS "${LV_SDL2_BIN_DIR}/SDL2.dll")
    add_custom_command(TARGET LineVerse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LV_SDL2_BIN_DIR}/SDL2.dll"
                "$<TARGET_FILE_DIR:LineVerse>"
        COMMENT "Copying SDL2 runtime DLL"
    )
endif()

if(WIN32 AND LV_SDL2_IMAGE_AVAILABLE AND EXISTS "${LV_SDL2_BIN_DIR}/SDL2_image.dll")
    add_custom_command(TARGET LineVerse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LV_SDL2_BIN_DIR}/SDL2_image.dll"
                "$<TARGET_FILE_DIR:LineVerse>"
        COMMENT "Copying SDL2_image runtime DLL"
    )
endif()

if(WIN32 AND LV_SDL2_TTF_AVAILABLE AND EXISTS "${LV_SDL2_BIN_DIR}/SDL2_ttf.dll")
    add_custom_command(TARGET LineVerse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LV_SDL2_BIN_DIR}/SDL2_ttf.dll"
                "$<TARGET_FILE_DIR:LineVerse>"
        COMMENT "Copying SDL2_ttf runtime DLL"
    )
endif()

if(WIN32 AND LV_SDL2_MIXER_AVAILABLE AND EXISTS "${LV_SDL2_BIN_DIR}/SDL2_mixer.dll")
    add_custom_command(TARGET LineVerse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LV_SDL2_BIN_DIR}/SDL2_mixer.dll"
                "$<TARGET_FILE_DIR:LineVerse>"
        COMMENT "Copying SDL2_mixer runtime DLL"
    )
endif()

# ==============================
# Summary
# ==============================
message(STATUS "================ LineVerse Build Summary ================")
message(STATUS "C++ standard          : ${CMAKE_CXX_STANDARD}")
message(STATUS "Asio root             : ${LV_ASIO_ROOT}")
message(STATUS "SDL2 root             : ${LV_SDL2_ROOT}")
if(WIN32)
    message(STATUS "SDL2 arch prefix      : ${LV_SDL2_PREFIX}")
endif()
message(STATUS "Build HandleGame      : ${LV_BUILD_HANDLE_GAME}")
message(STATUS "Build IdiomChainGame  : ${LV_BUILD_IDIOM_CHAIN_GAME}")
message(STATUS "Build PoetryRebuild   : ${LV_BUILD_POETRY_REBUILD_GAME}")
message(STATUS "Build VerseUnfold     : ${LV_BUILD_VERSE_UNFOLD_GAME}")

if(LV_SDL2_IMAGE_AVAILABLE)
    set(LV_HAS_SDL2_IMAGE "YES")
else()
    set(LV_HAS_SDL2_IMAGE "NO")
endif()

if(LV_SDL2_TTF_AVAILABLE)
    set(LV_HAS_SDL2_TTF "YES")
else()
    set(LV_HAS_SDL2_TTF "NO")
endif()

if(LV_SDL2_MIXER_AVAILABLE)

    set(LV_HAS_SDL2_MIXER "YES")
else()
    set(LV_HAS_SDL2_MIXER "NO")
endif()

message(STATUS "SDL2_image available  : ${LV_HAS_SDL2_IMAGE}")
message(STATUS "SDL2_ttf available    : ${LV_HAS_SDL2_TTF}")
message(STATUS "SDL2_mixer available  : ${LV_HAS_SDL2_MIXER}")
message(STATUS "=========================================================")