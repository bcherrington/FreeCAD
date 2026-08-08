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

if(TARGET PartGui)
    target_link_libraries(GpuDiagnosticsHarness_run PRIVATE PartGui)
    target_compile_definitions(
        GpuDiagnosticsHarness_run PRIVATE FREECAD_GPU_DIAGNOSTICS_HAS_PART
    )
endif()

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

    if(TARGET PartGui)
        add_test(
            NAME GpuDiagnosticsHarness_brep_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --samples 8
                --json
                --screenshot ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_x11.png
                --auto-exit-ms 2000
        )
        set_tests_properties(GpuDiagnosticsHarness_brep_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 8
        )

        foreach(edge_aa_mode
            hide
            line-smooth
            line-smooth-off
            screen-space-debug
            screen-space-only
            screen-space-overlay
            suppress-overlays
        )
            set(edge_aa_mode_fixture_args)
            if(edge_aa_mode STREQUAL "suppress-overlays")
                list(APPEND edge_aa_mode_fixture_args --brep-overlays)
            endif()
            add_test(
                NAME GpuDiagnosticsHarness_brep_${edge_aa_mode}_x11
                COMMAND GpuDiagnosticsHarness_run
                    --brep-fixture
                    ${edge_aa_mode_fixture_args}
                    --edge-aa-mode ${edge_aa_mode}
                    --samples 8
                    --json
                    --auto-exit-ms 2000
            )
            set_tests_properties(
                GpuDiagnosticsHarness_brep_${edge_aa_mode}_x11 PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=xcb"
                TIMEOUT 8
            )
        endforeach()
    endif()
endif()
