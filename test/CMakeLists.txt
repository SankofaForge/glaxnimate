# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-FileCopyrightText: 2023 Julius Künzel <jk.kdedev@smartlab.uber.space>
# SPDX-License-Identifier: BSD-2-Clause

include(ECMAddTests)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)

set(TESTS_LINK_LIBS
    Qt${QT_MAJOR_VERSION}::Test
    Qt${QT_MAJOR_VERSION}::Widgets
    KF${KF_MAJOR}::I18n
    QtColorWidgets
    Glaxnimate::Core
)

# Single source file tests, linking only to default libs

ecm_add_tests(
    test_bezier_solver.cpp
    test_riff.cpp
    test_property.cpp
    test_trim_path.cpp
    test_aep_gradient_xml.cpp
    test_animatable.cpp
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

# Multi source files tests

ecm_add_test(
    test_bezier_length.cpp
    TEST_NAME test_bezier_length
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_keyframe_transition.cpp
    TEST_NAME test_keyframe_transition
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_cos_parser.cpp
    TEST_NAME test_cos_parser
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_svg_renderer.cpp
    TEST_NAME test_svg_renderer
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

if ( GLAXNIMATE_WEBEXPORT_ENABLED )
    ecm_add_test(
        test_interactive_svg.cpp
        TEST_NAME test_interactive_svg
        LINK_LIBRARIES ${TESTS_LINK_LIBS}
    )
    ecm_add_test(
        test_css_svg.cpp
        TEST_NAME test_css_svg
        LINK_LIBRARIES ${TESTS_LINK_LIBS}
    )
    ecm_add_test(
        test_mobile_formats.cpp
        TEST_NAME test_mobile_formats
        LINK_LIBRARIES ${TESTS_LINK_LIBS}
    )
endif()

if ( NOT ANDROID )
    ecm_add_test(
        test_trace.cpp
        LINK_LIBRARIES ${TESTS_LINK_LIBS} Glaxnimate::Trace
    )
endif()
