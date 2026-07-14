# ===========================================================================
# CMakeLists.txt – Desktop (Windows) build for GLPIFrameworkIntro-Modern
#
# Shared C++ source is in Scene/. The same Scene/ code is used by:
#   - This file          → native desktop executable (GLFW + GLEW)
#   - android/.../cpp/   → Android shared library (.so) via NDK
#   - build_web.sh       → WebAssembly/WebGL via Emscripten
#
# Usage:
#   cmake -S . -B build_desktop -G "Visual Studio 17 2022" -A x64
#   cmake --build build_desktop --config Debug
# ===========================================================================

cmake_minimum_required(VERSION 3.15)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

cmake_policy(VERSION 3.15)

project(fan3d)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ImportDependencies)
importDependencies()

add_definitions(-DPLATFORM_WINDOWS -DGLEW_STATIC)

# ---------------------------------------------------------------------------
# Source files – everything in Scene/ (shared with Android + WebGL)
# ---------------------------------------------------------------------------
set(SCENE_DIR ${CMAKE_CURRENT_LIST_DIR}/Scene)
set(SCENE_SRC
        "${SCENE_DIR}/main.cpp"
        "${SCENE_DIR}/Renderer.cpp"
        "${SCENE_DIR}/Transform.cpp"
        "${SCENE_DIR}/Fan.cpp"
)

add_executable(fan3d ${SCENE_SRC})

target_include_directories(fan3d PRIVATE ${SCENE_DIR})

target_link_libraries(fan3d
        PRIVATE glfw
        PRIVATE libglew_static
        PRIVATE glm::glm
)

if(MSVC)
    target_compile_options(fan3d PRIVATE /W4 /WX- /permissive- /std:c++17)
endif()

# ---------------------------------------------------------------------------
# Post-build: copy the GLSL shaders next to the executable
# ---------------------------------------------------------------------------
add_custom_command(TARGET fan3d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/android/app/src/main/assets"
        "$<TARGET_FILE_DIR:fan3d>/assets"
        COMMENT "Copying shader assets to build directory"
)

# Tell Visual Studio to use the exe's output directory as the working
# directory when debugging, so the relative "assets/shader/" paths work.
set_target_properties(fan3d PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:fan3d>"
)

