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

        add_test(
            NAME GpuDiagnosticsHarness_brep_live-owned-fbo-force_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-overlays
                --samples 4
                --scene-aa-live
                --scene-aa-live-force
                --scene-aa-live-stats
                --json
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_live-owned-fbo-force_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_LIVE_STATS state=active reason=forced requested_samples=4 widget_sample_buffers=(0 widget_samples=0|[1-9][0-9]* widget_samples=4) owned_sample_buffers=[1-9][0-9]* actual_samples=[1-9][0-9]*"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_live-owned-fbo-force-document_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-overlays
                --samples 4
                --scene-aa-live
                --scene-aa-live-force
                --scene-aa-live-stats
                --frame-stats
                --screenshot
                    ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_live-owned-fbo-force-document_x11.png
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_live-owned-fbo-force-document_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_SCENE_AA_LIVE_STATS state=active reason=forced requested_samples=4 widget_sample_buffers=(0 widget_samples=0|[1-9][0-9]* widget_samples=4) owned_sample_buffers=[1-9][0-9]* actual_samples=[1-9][0-9]*"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-native_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-overlays
                --samples 4
                --frame-stats
                --json
                --screenshot
                    ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_brep_document-native_x11.png
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-native_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_FIXTURE revision=009-document-v1 composition=RoundedHousing,RoundedBoss,RoundedTransparentCanopy,HiddenRoundedSolid selected=RoundedBoss preselected=RoundedHousing hidden=HiddenRoundedSolid transparent=RoundedTransparentCanopy.*FREECAD_BREP_DOCUMENT_FRAME visible_center_pixels=[0-9]+ hidden_red_pixels=[0-9]+ navigation_cube_pixels=[0-9]+ selection_pixels=[0-9]+ preselection_pixels=[0-9]+ canopy_overlap_pixels=[0-9]+"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-tessellation-profile_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-tessellation
                --samples 4
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-tessellation-profile_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_TESSELLATION_RELATION outcome=pass baseline=[a-z_]+ finer=[a-z_]+ angle=[a-z_]+ scale=[a-z_]+ triangle_cap=3000000 memory_cap_bytes=67108864 time_cap_us=1200000 baseline_triangles=[0-9]+ finer_triangles=[0-9]+ angle_triangles=[0-9]+ scale_triangles=[0-9]+ cap_probe_fallback=triangle_cap cap_probe_triangles=[0-9]+ fallback_count=1"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-lighting-material_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-lighting-material
                --samples 4
                --json
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-lighting-material_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_LIGHTING_MATERIAL_RELATION outcome=pass baseline=1 lighting=1 material=1 combined=1 restore_match=1 camera_stable=1 visibility_stable=1 material_restore=1 light_restore=1 light_preferences_stable=1"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-transparency-order_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-transparency-order
                --samples 4
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-transparency-order_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_TRANSPARENCY_RELATION outcome=pass witness=009-document-v1 selected=RoundedBoss preselected=RoundedHousing hidden=HiddenRoundedSolid transparent=RoundedTransparentCanopy fallback=native production_enabled=0"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-hidden-lines-transparent_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-hidden-lines-transparent
                --samples 4
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-hidden-lines-transparent_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_DEPTH_STYLE_RELATION outcome=pass witness=009-document-v1 selected=RoundedBoss preselected=RoundedHousing hidden=HiddenRoundedSolid transparent=RoundedTransparentCanopy fallback=native production_enabled=0 wireframe=0 prepass_completed=1 prepass_callbacks=[1-9][0-9]* prepass_delta=[1-9][0-9]* hidden_prepass_witness=[0-9]+ transparent_surface=1 transparent_depth_prepass=1 visible_mode=visible visible_depth_func=lequal visible_depth_write=1 visible_style=solid-dark visible_witness=[1-9][0-9]* visible_color_pixels=[1-9][0-9]* visible_color_intensity=[1-9][0-9]* hidden_mode=hidden_dashed hidden_depth_func=greater hidden_depth_write=0 hidden_style=dashed-light hidden_witness=[1-9][0-9]* hidden_color_pixels=[1-9][0-9]* hidden_solid_mode=hidden_solid hidden_solid_style=solid-bright hidden_solid_witness=[1-9][0-9]* hidden_solid_color_pixels=[1-9][0-9]* hidden_control_mode=hidden_no_prepass hidden_control_witness=[0-9]+ display_mode_stable=1 visibility_stable=1 selection_stable=1 camera_stable=1 material_restore=1 restore_match=1 restore_pixel_diff=0 restored_render_us=[1-9][0-9]*"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-hidden-lines-transparent-preview_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-hidden-lines-transparent
                --brep-document-hidden-lines-preview
                --samples 4
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-hidden-lines-transparent-preview_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_HIDDEN_LINES_PREVIEW .*outcome=active .*profile=transparent .*fixture=009-document-v1 .*baseline=009-document-v1 .*visible_witness=[1-9][0-9]* .*hidden_witness=[1-9][0-9]* .*visible_style=cube-front-edges-solid-red .*hidden_style=cube-back-edges-outlined-dash .*transparent_surface=1 .*transparent_depth_prepass=1 .*implicit_dialog=0"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-hidden-lines-gallery_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-hidden-lines-transparent
                --brep-document-hidden-lines-gallery
                    ${CMAKE_CURRENT_BINARY_DIR}/GpuDiagnosticsHarness_hidden-lines-gallery
                --samples 4
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-hidden-lines-gallery_x11 PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=xcb"
                TIMEOUT 15
                PASS_REGULAR_EXPRESSION
                    "FREECAD_BREP_DOCUMENT_HIDDEN_LINES_GALLERY outcome=pass profile=transparent .*views=4 images=8 index=index.html fallback=native production_enabled=0"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-hidden-lines-wireframe_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-hidden-lines-wireframe
                --samples 4
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-hidden-lines-wireframe_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_DEPTH_STYLE_RELATION outcome=pass witness=009-document-v1 selected=RoundedBoss preselected=RoundedHousing hidden=HiddenRoundedSolid transparent=RoundedTransparentCanopy fallback=native production_enabled=0 wireframe=1 prepass_completed=1 prepass_callbacks=[1-9][0-9]* prepass_delta=[1-9][0-9]* hidden_prepass_witness=[0-9]+ transparent_surface=0 transparent_depth_prepass=0 visible_mode=visible visible_depth_func=lequal visible_depth_write=1 visible_style=solid-dark visible_witness=[1-9][0-9]* visible_color_pixels=[1-9][0-9]* visible_color_intensity=[1-9][0-9]* hidden_mode=hidden_dashed hidden_depth_func=greater hidden_depth_write=0 hidden_style=dashed-light hidden_witness=[1-9][0-9]* hidden_color_pixels=[1-9][0-9]* hidden_solid_mode=hidden_solid hidden_solid_style=solid-bright hidden_solid_witness=[1-9][0-9]* hidden_solid_color_pixels=[1-9][0-9]* hidden_control_mode=hidden_no_prepass hidden_control_witness=[0-9]+ display_mode_stable=1 visibility_stable=1 selection_stable=1 camera_stable=1 material_restore=1 restore_match=1 restore_pixel_diff=0 restored_render_us=[1-9][0-9]*"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-hidden-lines-wireframe-preview_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-hidden-lines-wireframe
                --brep-document-hidden-lines-preview
                --samples 4
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-hidden-lines-wireframe-preview_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_HIDDEN_LINES_PREVIEW .*outcome=active .*profile=wireframe .*fixture=009-document-v1 .*baseline=009-document-v1 .*visible_witness=[1-9][0-9]* .*hidden_witness=[1-9][0-9]* .*visible_style=cube-front-edges-solid-red .*hidden_style=cube-back-edges-outlined-dash .*transparent_surface=0 .*transparent_depth_prepass=0 .*implicit_dialog=0"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-depth-quantization_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-depth-quantization
                --samples 4
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-depth-quantization_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_DEPTH_QUANTIZATION_RELATION outcome=pass fallback=native production_enabled=0"
        )

        add_test(
            NAME GpuDiagnosticsHarness_topology-identity_x11
            COMMAND GpuDiagnosticsHarness_run
                --topology-identity
                --samples 4
                --auto-exit-ms 5000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_topology-identity_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 12
            PASS_REGULAR_EXPRESSION
                "FREECAD_SEGMENT_TOPOLOGY_IDENTITY_PROBE outcome=pass"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-postprocess-depth_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-postprocess-depth
                --samples 4
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-postprocess-depth_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_POSTPROCESS_DEPTH_RELATION outcome=pass requested=960x640 available=1 fallback=none reason=none gamma_policy=unmanaged-linear color_internal=GL_RGBA8 depth_internal=GL_DEPTH_COMPONENT24 depth_bits=24 color_encoding=GL_LINEAR depth_attachment_type=GL_TEXTURE sampleable_depth=1 no_pixel_readback=1"
        )

        add_test(
            NAME GpuDiagnosticsHarness_brep_document-postprocess-depth-force-native_x11
            COMMAND GpuDiagnosticsHarness_run
                --brep-fixture
                --brep-document-fixture
                --brep-document-postprocess-depth
                --brep-document-postprocess-depth-force-native
                --samples 4
                --auto-exit-ms 3000
        )
        set_tests_properties(
            GpuDiagnosticsHarness_brep_document-postprocess-depth-force-native_x11 PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=xcb"
            TIMEOUT 10
            PASS_REGULAR_EXPRESSION
                "FREECAD_BREP_DOCUMENT_POSTPROCESS_DEPTH_RELATION outcome=pass requested=960x640 available=0 fallback=native reason=forced gamma_policy=unmanaged-linear color_internal=GL_RGBA8 depth_internal=GL_DEPTH_COMPONENT24 depth_bits=24 color_encoding=unknown depth_attachment_type=none sampleable_depth=0 no_pixel_readback=1"
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

# Keep automated harness windows visible for real X11/OpenGL coverage without
# allowing either the viewport or an explicitly requested dialog to take focus.
# Manual harness launches remain normally interactive.
get_property(gui_registered_tests DIRECTORY PROPERTY TESTS)
foreach(gui_test_name IN LISTS gui_registered_tests)
    if(gui_test_name MATCHES "^GpuDiagnosticsHarness_")
        set_property(TEST ${gui_test_name} APPEND PROPERTY ENVIRONMENT
            "FREECAD_GPU_DIAGNOSTICS_PASSIVE_WINDOWS=1"
        )
    endif()
endforeach()
