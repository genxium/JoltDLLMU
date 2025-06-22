cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(JoltDLLMU_cpp_test VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

set(JOLT_BINDINGS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../JoltBindings)
set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../joltphysics_v5.3.0)
set(PREBUILT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../UnityPackageOutput/Runtime/Plugins)
set(PREBUILT_HEADER_PATH ${PREBUILT_LIB_PATH}/include)

set(TARGET_NAME cpp_test)

set(CPP_TEST_SRC_FILES 
    Test.cpp
)

add_executable(${TARGET_NAME} ${CPP_TEST_SRC_FILES})

target_include_directories(${TARGET_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${PREBUILT_HEADER_PATH}>
	$<INSTALL_INTERFACE:/include>)

find_library(JOLTC_DLL_LIB NAMES joltc PATHS ${PREBUILT_LIB_PATH})
add_library(JOLTC_DLL_LIB SHARED IMPORTED)

# [WARNING] Assuming that "protoc" is installed by "vcpkg install protobuf protobuf:x64-windows".
set(protobuf_DIR "$ENV{VCPKG_INSTALLED_SHARE}/protobuf")
find_package(protobuf CONFIG REQUIRED)

target_link_libraries(${TARGET_NAME} PUBLIC 
        protobuf::libprotobuf
        ${JOLTC_DLL_LIB}
)

# Set the debug/non-debug build flags
target_compile_definitions(${TARGET_NAME} PUBLIC "$<$<CONFIG:Debug>:_DEBUG>")
target_compile_definitions(${TARGET_NAME} PUBLIC "$<$<CONFIG:Release,Distribution,ReleaseASAN,ReleaseUBSAN,ReleaseTSAN,ReleaseCoverage>:NDEBUG>")

# ---------------------------- Installation ----------------------------
install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf
    DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Dependencies
)

install(FILES 
    ${PREBUILT_LIB_PATH}/joltc.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Dependencies
)

install(FILES ${PREBUILT_LIB_PATH}/joltc.pdb DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)

# For Visual Studio to pick up runtime dependent DLLs
install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG> COMPONENT Dependencies
)

install(FILES 
    ${PREBUILT_LIB_PATH}/joltc.dll
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG> COMPONENT Dependencies
)
