# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Dialogs)
add_subdirectory(propertyeditor)

# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        Camera.cpp
        ColorScaleOverlayTest.cpp
        FileDialog.cpp
        GpuDiagnostics.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParameterManagerTest.cpp
        StyleParameters/YamlParameterSourceTest.cpp
        InputHintTest.cpp
        SelectionTest.cpp
)

add_executable(GpuDiagnosticsHarness_run
    GpuDiagnosticsHarness.cpp
)

# Qt tests
setup_qt_test(QuantitySpinBox)

target_link_libraries(Gui_tests_run PRIVATE
    GTest::gmock_main
    FreeCADApp
    FreeCADGui
)

target_link_libraries(GpuDiagnosticsHarness_run PRIVATE
    FreeCADApp
    FreeCADGui
)

set_target_properties(GpuDiagnosticsHarness_run PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(
    NAME GpuDiagnosticsHarness_dialog_offscreen
    COMMAND GpuDiagnosticsHarness_run --samples 8 --dialog --auto-exit-ms 1000
)
set_tests_properties(GpuDiagnosticsHarness_dialog_offscreen PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 5
)

if(UNIX AND NOT APPLE AND DEFINED ENV{DISPLAY} AND NOT "$ENV{DISPLAY}" STREQUAL "")
    add_test(
        NAME GpuDiagnosticsHarness_dialog_x11
        COMMAND GpuDiagnosticsHarness_run --samples 8 --dialog --auto-exit-ms 1000
    )
    set_tests_properties(GpuDiagnosticsHarness_dialog_x11 PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=xcb"
        TIMEOUT 5
    )
endif()
