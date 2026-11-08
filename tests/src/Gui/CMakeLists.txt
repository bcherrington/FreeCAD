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
setup_qt_test(ThreadAffinity)
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
