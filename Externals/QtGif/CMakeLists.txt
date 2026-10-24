# The bundled Qt build disables GIF. Build the matching, unmodified Qt reader as a plugin.
add_library(orbit-qt-gif MODULE main.cpp qgifhandler.cpp qgifhandler_p.h gif.json)
set_target_properties(orbit-qt-gif PROPERTIES
  AUTOMOC ON
  OUTPUT_NAME qgif
  DEBUG_POSTFIX d
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Binaries/QtPlugins/imageformats"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Binaries/QtPlugins/imageformats"
)
target_include_directories(orbit-qt-gif PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(orbit-qt-gif PRIVATE Qt6::Gui)
add_custom_command(TARGET orbit-qt-gif POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES"
          "${CMAKE_BINARY_DIR}/Binaries/QtGif-Licenses"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
          "${CMAKE_BINARY_DIR}/Binaries/QtGif-Licenses/README.md"
)
