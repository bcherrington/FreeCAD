# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Dialogs)
add_subdirectory(propertyeditor)

# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        BitmapFactory.cpp
        Camera.cpp
        ColorScaleOverlayTest.cpp
        FileDialog.cpp
        StyleParameters/DiagnosticsCapture.h
        StyleParameters/ValueMatchers.h
        StyleParameters/ValueMatchers.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParserRobustnessTest.cpp
        StyleParameters/ParameterManagerTest.cpp
        StyleParameters/YamlParameterSourceTest.cpp
        StyleParameters/DiagnosticsTest.cpp
        StyleParameters/ValueAccessTest.cpp
        StyleParameters/EdgesTest.cpp
        StyleParameters/CornersTest.cpp
        StyleParameters/GradientTest.cpp
        StyleParameters/ColorShadingTest.cpp
        InputHintTest.cpp
        ViewProviderDoubleClick.cpp
        NativeGesturePinchTest.cpp
        NavigationStylePinchTest.cpp
        NavigationStyleWheelTest.cpp
        SelectionTest.cpp
        SoMouseWheelEventTest.cpp
)

# Qt tests
# TreeWidget is internal and has no Windows DLL export; keep this test off Windows.
if(NOT WIN32)
    setup_qt_test(TreeWidgetLifetime)
    setup_qt_test(TreeRestore)
    setup_qt_test(TreeBehavior)
    target_compile_definitions(TreeBehavior_Tests_run PRIVATE
        TREE_BEHAVIOR_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    set_tests_properties(TreeBehavior_Tests_run PROPERTIES TIMEOUT 300)
    set_tests_properties(TreeRestore_Tests_run PROPERTIES TIMEOUT 120)
    set_tests_properties(TreeWidgetLifetime_Tests_run PROPERTIES TIMEOUT 120)
endif()
setup_qt_test(CompactMainWindowChrome)
setup_qt_test(ObjectSelection)
setup_qt_test(ImagePlaneRender)
set_tests_properties(ImagePlaneRender_Tests_run PROPERTIES TIMEOUT 180)
setup_qt_test(ImagePlane)
set_tests_properties(ImagePlane_Tests_run PROPERTIES TIMEOUT 120)
setup_qt_test(TranslatorLocale)
setup_qt_test(InputField)
setup_qt_test(QuantitySpinBox)
setup_qt_test(SelectionView)
setup_qt_test(TreeWidgetLifecycle)
set_tests_properties(TreeWidgetLifecycle_Tests_run PROPERTIES TIMEOUT 120)
foreach(_case IN ITEMS liveRestoreCompletion finishOnUnwind survivingTreeAndPendingDeletion)
    add_test(NAME TreeWidgetLifecycle_${_case} COMMAND TreeWidgetLifecycle_Tests_run ${_case})
    set_tests_properties(TreeWidgetLifecycle_${_case} PROPERTIES
        TIMEOUT 120
        LABELS "Qt"
        ENVIRONMENT "${_qt_test_environment}")
    if(WIN32)
        set_tests_properties(TreeWidgetLifecycle_${_case} PROPERTIES
            ENVIRONMENT_MODIFICATION "${_test_env_mod}")
    endif()
endforeach()
# Each lifecycle data row owns a fresh native process and private fixture profile.
# Keep the pre-fix receipts; these are ordinary regression entries after T008 repair.
set(_spec012_tree_selections
    spec012TreeUnarmedControl
    spec012TreeClaimMutation
    spec012DeferredControl
    spec012DeferredMutation:post-recompute-immediate-convergence
    spec012DeferredMutation:native-execute-nested-loop-real-requeue
    spec012PropertyGeneration:unarmed-property-control
    spec012PropertyGeneration:remove-readd-later-property
    spec012ObjectGeneration:unarmed-object-control
    spec012ObjectGeneration:replace-later-object-name
    spec012DocumentNameReuse:own-deferred-work-control
    spec012DocumentNameReuse:close-recreate-document-name
    spec012NativeDestruction:non-destructive-native-callback
    spec012NativeDestruction:delete-current-provider
    spec012NativeDestruction:close-current-document
    spec012VisibleLifecycle:visible-native-control
    spec012VisibleLifecycle:visible-delete-current-provider
    spec012VisibleLifecycle:visible-close-current-document
    spec012VisibleLifecycle:visible-delete-then-throw
    spec012VisibleLifecycle:visible-ordinary-exception
    spec012CurrentPropertyGeneration:current-property-control
    spec012CurrentPropertyGeneration:remove-readd-current-property
    spec012TreeNewObject:new-object-before-snapshot-control
    spec012TreeNewObject:claim-creates-object-after-snapshot
    spec012ChildClaimLifecycle:child-claim-control
    spec012ChildClaimLifecycle:child-claim-delete-provider
    spec012ChildClaimLifecycle:child-claim-close-document
    spec012EditSubObjectLifecycle:edit-subobject-control
    spec012EditSubObjectLifecycle:edit-subobject-reset-edit
    spec012EditSubObjectLifecycle:edit-subobject-delete-provider
    spec012EditSubObjectLifecycle:edit-subobject-close-document
    spec012RetainedProviderUndo:retained-undo-unarmed-control
    spec012RetainedProviderUndo:retained-undo-original-bit-false
    spec012RetainedProviderUndo:retained-undo-original-bit-true
    spec012GuiTransactionPropertyCleanup:owned-gui-link-saved-copy-cleanup
    spec012GuiTransactionPropertyCleanup:borrowed-gui-property-rename-cleanup)
foreach(_selection IN LISTS _spec012_tree_selections)
    string(REPLACE ":" "_" _case "${_selection}")
    add_test(NAME TreeWidgetLifecycle_${_case}
        COMMAND TreeWidgetLifecycle_Tests_run "${_selection}")
    set_tests_properties(TreeWidgetLifecycle_${_case} PROPERTIES
        TIMEOUT 120
        LABELS "Qt;spec012;B035"
        ENVIRONMENT "${_qt_test_environment}")
    if(WIN32)
        set_tests_properties(TreeWidgetLifecycle_${_case} PROPERTIES
            ENVIRONMENT_MODIFICATION "${_test_env_mod}")
    endif()
endforeach()
unset(_spec012_tree_selections)
setup_qt_test(ThreadAffinity)
setup_qt_test(DocumentUpdates)
# Bound the test itself so direct CTest invocations cannot hang indefinitely.
set_tests_properties(ThreadAffinity_Tests_run PROPERTIES TIMEOUT 120)
setup_qt_test(NumericLocaleConsumers)

target_link_libraries(Gui_tests_run PRIVATE
    GTest::gmock_main
    ${Python3_LIBRARIES}
    FreeCADApp
    FreeCADGui
)

if(WIN32)
    target_compile_definitions(Gui_tests_run PRIVATE QUARTER_DLL)
endif()

option(BUILD_IMAGE_PERFORMANCE_PROBE "Build the opt-in Spec 003 image benchmark probe" OFF)
if(BUILD_IMAGE_PERFORMANCE_PROBE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(ImageConversionPerformanceProbe ImageConversionPerformanceProbe.cpp)
        target_link_libraries(ImageConversionPerformanceProbe PRIVATE FreeCADGui FreeCADApp ${Python3_LIBRARIES})
        keep_python_linked(ImageConversionPerformanceProbe)
        set_target_properties(ImageConversionPerformanceProbe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    endif()
    setup_qt_test(ImagePerformanceTraceTest)
    target_compile_definitions(ImagePlane_Tests_run PRIVATE FC_IMAGE_PERFORMANCE_TRACE)
    target_sources(FreeCADGui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ImagePerformanceTrace.cpp)
    set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/src/Gui/BitmapFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/Gui/ViewProviderImagePlane.cpp
        DIRECTORY ${PROJECT_SOURCE_DIR}/src/Gui
        PROPERTIES
            COMPILE_DEFINITIONS FC_IMAGE_PERFORMANCE_TRACE
            INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
