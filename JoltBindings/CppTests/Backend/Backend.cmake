set(BACKEND_TEST_ROOT ${JOLT_BINDINGS_ROOT}/CppTests/Backend)

# Source files
set(BACKEND_TEST_SRC_FILES
	${BACKEND_TEST_ROOT}/Test.cpp
)

# Group source files
source_group(TREE ${BACKEND_TEST_ROOT} FILES ${BACKEND_TEST_SRC_FILES})

add_executable(BackendTest ${BACKEND_TEST_SRC_FILES})
target_link_libraries(BackendTest LINK_PUBLIC ${TARGET_NAME})

target_link_libraries(BackendTest PUBLIC 
    protobuf::libprotobuf
)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BackendTest)
    target_link_options(BackendTest PUBLIC "/SUBSYSTEM:CONSOLE")
endif()

target_include_directories(BackendTest PUBLIC
	$<BUILD_INTERFACE:${PHYSICS_REPO_ROOT}>
    $<BUILD_INTERFACE:${JOLT_BINDINGS_ROOT}/joltc>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}> # for generated header
    $<BUILD_INTERFACE:${PB_GEN_ROOT}> # For pb class headers
    $<INSTALL_INTERFACE:/include>)

set(MY_RUNTIME_DEPS_DESTINATIONS "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>") # [WARNING] Intentionally NOT installing to "UnityPackageOutput" folder.

foreach (_rt_deps_destination ${MY_RUNTIME_DEPS_DESTINATIONS}) 
    if (MSVC)
        install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf 
            DESTINATION ${_rt_deps_destination} COMPONENT Dependencies
        )
        install(FILES $<TARGET_PDB_FILE:BackendTest> DESTINATION ${_rt_deps_destination} OPTIONAL)
    else()    
        foreach (_file ${Protobuf_LIBRARIES}) # Magic variable set by "FindProtobuf -- find_package(Protobuf REQUIRED)". See https://cmake.org/cmake/help/latest/module/FindProtobuf.html#result-variables.
            get_filename_component(_resolvedFile "${_file}" REALPATH)
            get_filename_component(_destinationFilename "${_file}" NAME)
            install(FILES ${_resolvedFile}
                DESTINATION ${_rt_deps_destination} COMPONENT Dependencies RENAME ${_destinationFilename}
            )
        endforeach()
    endif()

    install(FILES ${OVERRIDE_INSTALL_DESTINATION}/PrimitiveConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
    install(FILES ${OVERRIDE_INSTALL_DESTINATION}/ConfigConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
endforeach()
