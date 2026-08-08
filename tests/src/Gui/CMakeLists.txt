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
        ProjectedSegmentDeduplication.cpp
        ScreenSpaceEdgeGeometry.cpp
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
            dedup-screen-space
            shader-screen-space
            shader-screen-space-dedup
            shader-screen-space-overlay
            screen-space-debug
            screen-space-only
            screen-space-overlay
            suppress-overlays
        )
            set(edge_aa_mode_fixture_args)
            if(edge_aa_mode STREQUAL "suppress-overlays")
                list(APPEND edge_aa_mode_fixture_args --brep-overlays)
            endif()
            if(edge_aa_mode STREQUAL "dedup-screen-space")
                list(APPEND edge_aa_mode_fixture_args
                    --brep-duplicate-edges
                    --edge-aa-dedup-tolerance 0.5
                    --edge-aa-stats
                )
            endif()
            if(edge_aa_mode MATCHES "^shader-screen-space")
                list(APPEND edge_aa_mode_fixture_args
                    --brep-duplicate-edges
                    --edge-aa-shader-width 1.5
                    --edge-aa-stats
                )
            endif()
            if(edge_aa_mode STREQUAL "shader-screen-space-dedup"
               OR edge_aa_mode STREQUAL "shader-screen-space-overlay")
                list(APPEND edge_aa_mode_fixture_args --edge-aa-dedup-tolerance 0.5)
            endif()
            if(edge_aa_mode STREQUAL "shader-screen-space-overlay")
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
            if(edge_aa_mode STREQUAL "dedup-screen-space")
                set_tests_properties(
                    GpuDiagnosticsHarness_brep_${edge_aa_mode}_x11 PROPERTIES
                    PASS_REGULAR_EXPRESSION
                        "FREECAD_EDGE_AA_DEDUP_STATS input=24 rejected=0 duplicate=12 output=12 tolerance_px=0.500"
                )
            endif()
            if(edge_aa_mode STREQUAL "shader-screen-space")
                set_tests_properties(
                    GpuDiagnosticsHarness_brep_${edge_aa_mode}_x11 PROPERTIES
                    PASS_REGULAR_EXPRESSION
                        "FREECAD_EDGE_AA_SHADER_STATS input=24 rejected=0 duplicate=0 output=24 vertices=144 draw_calls=1 dpr=1.000 width_logical=1.500 width_physical=1.500 dedup=0 shader_ready=1"
                )
            endif()
            if(edge_aa_mode STREQUAL "shader-screen-space-dedup"
               OR edge_aa_mode STREQUAL "shader-screen-space-overlay")
                set_tests_properties(
                    GpuDiagnosticsHarness_brep_${edge_aa_mode}_x11 PROPERTIES
                    PASS_REGULAR_EXPRESSION
                        "FREECAD_EDGE_AA_SHADER_STATS input=24 rejected=0 duplicate=12 output=12 vertices=72 draw_calls=1 dpr=1.000 width_logical=1.500 width_physical=1.500 dedup=1 shader_ready=1"
                )
            endif()
        endforeach()
    endif()
endif()
