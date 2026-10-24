cmake_minimum_required(VERSION 3.19)
project(OrbitHomeSmoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
get_filename_component(DOLPHIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
list(PREPEND CMAKE_PREFIX_PATH "${DOLPHIN_SOURCE}/Externals/Qt/Qt6.8.3/x64")
find_package(Qt6 6.8 COMPONENTS Widgets REQUIRED)
add_subdirectory("${DOLPHIN_SOURCE}/Externals/QtGif" "${CMAKE_CURRENT_BINARY_DIR}/QtGif")

add_executable(orbit-home-smoke
  smoke.cpp
  background.cpp
  shell.cpp
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/HomeWidget.cpp"
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/HomeWidget.h"
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/OrbitBackground.cpp"
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/OrbitBackground.h"
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/OrbitTitleBar.cpp"
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/OrbitTitleBar.h"
)
add_dependencies(orbit-home-smoke orbit-qt-gif)
target_include_directories(orbit-home-smoke PRIVATE
  "${DOLPHIN_SOURCE}/Source/Core"
  "${DOLPHIN_SOURCE}/Externals/fmt/fmt/include"
)
target_link_libraries(orbit-home-smoke PRIVATE Qt6::Widgets)
if(MSVC)
  target_compile_options(orbit-home-smoke PRIVATE /utf-8 /permissive- /Zc:__cplusplus /Zc:preprocessor)
endif()
target_compile_definitions(orbit-home-smoke PRIVATE
  ORBIT_PREVIEW_PATH="${CMAKE_CURRENT_BINARY_DIR}/orbit-preview.png"
  ORBIT_EMPTY_PREVIEW_PATH="${CMAKE_CURRENT_BINARY_DIR}/orbit-empty-preview.png"
  ORBIT_PLUGIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/Binaries/QtPlugins"
  ORBIT_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/background.gif"
)
