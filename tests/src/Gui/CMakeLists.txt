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

        foreach(scene_aa_samples 0 8)
            add_test(
                NAME GpuDiagnosticsHarness_brep_owned-fbo-${scene_aa_samples}x_x11
                COMMAND GpuDiagnosticsHarness_run
                    --brep-fixture
                    --samples 8
                    --scene-aa-samples ${scene_aa_samples}
                    --scene-aa-stats
                    --json
                    --screenshot
                        ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_owned-fbo-${scene_aa_samples}x_x11.png
                    --auto-exit-ms 2000
            )
            set_tests_properties(
                GpuDiagnosticsHarness_brep_owned-fbo-${scene_aa_samples}x_x11 PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=xcb"
                TIMEOUT 8
                PASS_REGULAR_EXPRESSION
                    "FREECAD_SCENE_AA_STATS method=owned-fbo requested_samples=${scene_aa_samples} format_samples=[0-9]+ sample_buffers=[0-9]+ actual_samples=[0-9]+ width=960 height=640 decorations=0"
            )
        endforeach()

        add_test(
            NAME GpuDiagnosticsHarness_brep_owned-fbo-8x-hidpi_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-overlays
                --samples 8
                --scene-aa-samples 8
                --scene-aa-stats
                --json
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_owned-fbo-8x-hidpi_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb;QT_SCALE_FACTOR=2"
            TIMEOUT 8
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_STATS method=owned-fbo requested_samples=8 format_samples=[0-9]+ sample_buffers=[0-9]+ actual_samples=[0-9]+ width=1920 height=1280 decorations=0"
        )

        foreach(scene_aa_live_samples 0 8)
            add_test(
                NAME GpuDiagnosticsHarness_brep_live-owned-fbo-${scene_aa_live_samples}x_x11
                COMMAND GpuDiagnosticsHarness_run
                    --brep-fixture
                    --brep-overlays
                    --samples ${scene_aa_live_samples}
                    --scene-aa-live
                    --scene-aa-live-stats
                    --json
                    --screenshot
                        ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_live-owned-fbo-${scene_aa_live_samples}x_x11.png
                    --auto-exit-ms 3000
            )
            if(scene_aa_live_samples EQUAL 0)
                set(scene_aa_live_expected_state
                    "state=fallback reason=requested-samples-zero requested_samples=0"
                )
            else()
                set(scene_aa_live_expected_state
                    "state=active reason=none requested_samples=8 widget_sample_buffers=0 widget_samples=0 owned_sample_buffers=[1-9][0-9]* actual_samples=[1-9][0-9]*"
                )
            endif()
            set_tests_properties(
                GpuDiagnosticsHarness_brep_live-owned-fbo-${scene_aa_live_samples}x_x11 PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=xcb"
                TIMEOUT 10
                PASS_REGULAR_EXPRESSION
                    "FREECAD_SCENE_AA_LIVE_STATS ${scene_aa_live_expected_state}.*width=960 height=640"
            )
        endforeach()

        add_test(
            NAME GpuDiagnosticsHarness_brep_live-owned-fbo-8x-hidpi_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-overlays
                --samples 8
                --scene-aa-live
                --scene-aa-live-stats
                --json
                --screenshot
                    ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_live-owned-fbo-8x-hidpi_x11.png
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_live-owned-fbo-8x-hidpi_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb;QT_SCALE_FACTOR=2"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_LIVE_STATS state=active reason=none requested_samples=8 widget_sample_buffers=0 widget_samples=0 owned_sample_buffers=[1-9][0-9]* actual_samples=[1-9][0-9]* width=1920 height=1280"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_live-owned-fbo-8x-resize_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-overlays
                --samples 8
                --scene-aa-live
                --scene-aa-live-resize
                --scene-aa-live-stats
                --json
                --screenshot
                    ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_live-owned-fbo-8x-resize_x11.png
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_live-owned-fbo-8x-resize_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_LIVE_STATS state=active reason=none requested_samples=8 widget_sample_buffers=0 widget_samples=0 owned_sample_buffers=[1-9][0-9]* actual_samples=[1-9][0-9]* width=800 height=600 allocations=[2-9][0-9]*"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_live-owned-fbo-8x-dense_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-dense-fixture
                --brep-overlays
                --samples 8
                --scene-aa-live
                --scene-aa-live-stats
                --frame-stats
                --json
                --screenshot
                    ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_live-owned-fbo-8x-dense_x11.png
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_live-owned-fbo-8x-dense_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_LIVE_STATS state=active reason=none requested_samples=8 widget_sample_buffers=0 widget_samples=0 owned_sample_buffers=[1-9][0-9]* actual_samples=[1-9][0-9]* width=960 height=640"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_live-owned-fbo-fallback-transition_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-overlays
                --samples 8
                --scene-aa-live
                --scene-aa-live-stats
                --scene-aa-live-fallback-transition
                --json
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_live-owned-fbo-fallback-transition_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_LIVE_STATS state=fallback reason=requested-samples-zero requested_samples=0.*owned_sample_buffers=0 actual_samples=0.*allocations=[1-9][0-9]*"
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
