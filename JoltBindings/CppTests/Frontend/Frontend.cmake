set(FRONTEND_TEST_ROOT ${JOLT_BINDINGS_ROOT}/CppTests/Frontend)

# Source files
set(FRONTEND_TEST_SRC_FILES
	${FRONTEND_TEST_ROOT}/Test.cpp
)

# Group source files
source_group(TREE ${FRONTEND_TEST_ROOT} FILES ${FRONTEND_TEST_SRC_FILES})

add_executable(FrontendTest ${FRONTEND_TEST_SRC_FILES})

target_compile_definitions(FrontendTest PRIVATE JPH_SHARED_LIBRARY) # [IMPORTANT] For correctly define the macro "JPH_EXPORT" as "__declspec(dllimport)"

target_link_libraries(FrontendTest LINK_PUBLIC ${TARGET_NAME})
if(USE_STATIC_PB) 
    target_link_libraries(FrontendTest PRIVATE 
        protobuf::libprotobuf
    )
else()
    target_link_libraries(FrontendTest PUBLIC 
        protobuf::libprotobuf
    )
endif()

if (MSVC)
    #set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FrontendTest)
    target_link_options(FrontendTest PUBLIC "/SUBSYSTEM:CONSOLE")
endif()

target_include_directories(FrontendTest PUBLIC
	$<BUILD_INTERFACE:${PHYSICS_REPO_ROOT}>
    $<BUILD_INTERFACE:${JOLT_BINDINGS_ROOT}/joltc>
    $<BUILD_INTERFACE:${PB_GEN_ROOT}>
    $<INSTALL_INTERFACE:/include>)

if (MSVC)
else ()
    set_target_properties(
        FrontendTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
        LIBRARY_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
        ARCHIVE_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
    )
endif ()

set(MY_RUNTIME_DEPS_DESTINATIONS "${OVERRIDE_BINARY_DESTINATION}") # [WARNING] Intentionally NOT installing to "UnityPackageOutput" folder.

foreach (_rt_deps_destination ${MY_RUNTIME_DEPS_DESTINATIONS}) 
    if (MSVC)
        install(FILES $<TARGET_PDB_FILE:FrontendTest> DESTINATION ${_rt_deps_destination} OPTIONAL)
    endif()

    install(FILES ${OVERRIDE_INSTALL_DESTINATION}/PrimitiveConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
    install(FILES ${OVERRIDE_INSTALL_DESTINATION}/ConfigConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
endforeach()
