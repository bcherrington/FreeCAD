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
