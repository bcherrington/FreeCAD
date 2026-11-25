# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(App_tests_run
        AsyncRecompute.cpp
        Application.cpp
        ApplicationDirectories.cpp
        ApplicationExportTypes.cpp
        BackupPolicy.cpp
        Branding.cpp
        ComplexGeoData.cpp
        Document.cpp
        DocumentObject.cpp
        DocumentObserver.cpp
        Expression.cpp
        ExpressionParser.cpp
        QuantityInput.cpp
        ElementMap.cpp
        ElementNamingUtils.cpp
        IndexedName.cpp
        License.cpp
        Link.cpp
        MappedElement.cpp
        MappedName.cpp
        Metadata.cpp
        ProjectFile.cpp
        PropertyFile.cpp
        Property.h
        Property.cpp
        PropertyXLink.cpp
        PropertyExpressionEngine.cpp
        StringHasher.cpp
        VarSet.cpp
        VRMLObject.cpp
)

target_compile_definitions(App_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_link_libraries(App_tests_run PRIVATE
    GTest::gmock_main
    ${Python3_LIBRARIES}
    FreeCADApp
)

# Exact-file I/O fault injection belongs to the native tests, never the application.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(IncludedFileIOFaults SHARED PropertyFileIOFaults.cpp)
    set_target_properties(IncludedFileIOFaults PROPERTIES CXX_VISIBILITY_PRESET default)
    target_link_libraries(IncludedFileIOFaults PRIVATE ${CMAKE_DL_LIBS})
    target_link_libraries(App_tests_run PRIVATE IncludedFileIOFaults)
    target_compile_definitions(App_tests_run PRIVATE FC_TEST_INCLUDED_FILE_IO_FAULTS)
endif()

option(BUILD_INCLUDED_FILE_PERFORMANCE_PROBE "Build the opt-in Spec 003 restore probe" OFF)
if(BUILD_INCLUDED_FILE_PERFORMANCE_PROBE)
    target_sources(FreeCADApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/IncludedFilePerformanceTrace.cpp)
    set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/src/App/PropertyFile.cpp
        DIRECTORY ${PROJECT_SOURCE_DIR}/src/App
        PROPERTIES
            COMPILE_DEFINITIONS FC_INCLUDED_FILE_PERFORMANCE_TRACE
            INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_definitions(App_tests_run PRIVATE FC_INCLUDED_FILE_PERFORMANCE_TRACE)
endif()

option(BUILD_DOCUMENT_PERFORMANCE_PROBE "Build opt-in save/parser performance trace" OFF)
if(BUILD_DOCUMENT_PERFORMANCE_PROBE)
    target_sources(FreeCADApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DocumentPerformanceTrace.cpp)
    foreach(source PropertyFile.cpp ElementMap.cpp)
        set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/App/${source}
            DIRECTORY ${PROJECT_SOURCE_DIR}/src/App APPEND PROPERTY
            COMPILE_DEFINITIONS FC_DOCUMENT_PERFORMANCE_TRACE)
        set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/App/${source}
            DIRECTORY ${PROJECT_SOURCE_DIR}/src/App APPEND PROPERTY
            INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach()
    target_sources(App_tests_run PRIVATE DocumentPerformanceTraceTest.cpp)
    target_compile_definitions(App_tests_run PRIVATE FC_DOCUMENT_PERFORMANCE_TRACE)
endif()

add_test(
    NAME PythonSignalHandlers
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/PythonSignalHandlers.py
            $<TARGET_FILE_DIR:FreeCADMainPy>
)
set_tests_properties(PythonSignalHandlers PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(PythonSignalHandlers PROPERTIES
        ENVIRONMENT_MODIFICATION "${_test_env_mod}")
endif()

if(BUILD_PART)
    add_test(
        NAME PythonModelingSignalHandlers
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/PythonSignalHandlers.py
                $<TARGET_FILE_DIR:FreeCADMainPy> $<TARGET_FILE_DIR:Part>
    )
    set_tests_properties(PythonModelingSignalHandlers PROPERTIES TIMEOUT 60)
    if(WIN32)
        set_tests_properties(PythonModelingSignalHandlers PROPERTIES
            ENVIRONMENT_MODIFICATION "${_test_env_mod}")
    endif()
endif()
