# SPDX-FileCopyrightText: 2026 Builder106
# SPDX-License-Identifier: BSD-2-Clause

# Embed the player script so exports work without external files
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/svg_player.js GLAXNIMATE_SVG_PLAYER_JS)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/svg_player.js)
configure_file(svg_player_js.in.hpp ${CMAKE_CURRENT_BINARY_DIR}/svg_player_js.hpp @ONLY)

set(SOURCES
    webexport_module.cpp
    interactive_svg_format.cpp
    css_svg_format.cpp
)

add_library(GlaxnimateWebExport OBJECT ${SOURCES})
target_include_directories(GlaxnimateWebExport PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
glaxnimate_enable_exceptions(GlaxnimateWebExport PUBLIC)
