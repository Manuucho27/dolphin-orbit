cmake_minimum_required(VERSION 3.19)
project(OrbitHomeSmoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
get_filename_component(DOLPHIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
list(PREPEND CMAKE_PREFIX_PATH "${DOLPHIN_SOURCE}/Externals/Qt/Qt6.8.3/x64")
find_package(Qt6 6.8 COMPONENTS Widgets REQUIRED)

add_executable(orbit-home-smoke
  smoke.cpp
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/HomeWidget.cpp"
  "${DOLPHIN_SOURCE}/Source/Core/DolphinQt/Home/HomeWidget.h"
)
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
)
