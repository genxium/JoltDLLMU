cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(LibExportImportCppTest VERSION 0.0.1 LANGUAGES CXX)

if ($ENV{USE_STATIC_PB} MATCHES "true") 
    set(USE_STATIC_PB ON)
    if (MSVC)
        # See comments in `<proj-root>/JoltBindings/CMakeLists.txt`.
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
    endif()
    message(STATUS "About to use static protobuf libs")
else()
    message(STATUS "About to use shared protobuf dlls")
endif()

# Requires C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if ($ENV{USE_STATIC_PB} MATCHES "true") 
    set(USE_STATIC_PB ON)
endif()

if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()

include(GNUInstallDirs)

if (WIN32)
else()
    set(CMAKE_CXX_COMPILER "/usr/bin/c++")
endif()

set(BUILD_TYPE $ENV{BUILD_TYPE})

set(JOLT_BINDINGS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../JoltBindings)
set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../joltphysics_v5.3.0)
set(PREBUILT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../UnityPackageOutput/Runtime/Plugins)
set(PREBUILT_HEADER_PATH ${PREBUILT_LIB_PATH}/include)

set(TARGET_NAME LibExportImportCppTest)

set(CPP_TEST_SRC_FILES 
    Test.cpp
    ${JOLT_BINDINGS_ROOT}/joltc/TestHelper.cpp
)

add_executable(${TARGET_NAME} ${CPP_TEST_SRC_FILES})
target_compile_definitions(${TARGET_NAME} PRIVATE JPH_SHARED_LIBRARY) # [IMPORTANT] For correctly define the macro "JPH_EXPORT" as "__declspec(dllimport)"
if(BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Turning on JPH_ASSERT for BUILD_TYPE=Debug")
    if(MSVC)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    endif()
else()
    message(STATUS "Turning off JPH_ASSERT for BUILD_TYPE=Release")
    target_compile_definitions(${TARGET_NAME} PUBLIC JPH_NO_DEBUG)
    target_compile_definitions(${TARGET_NAME} PUBLIC NDEBUG)
endif()

target_include_directories(${TARGET_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${PREBUILT_HEADER_PATH}>
	$<INSTALL_INTERFACE:/include>)

message(STATUS "Finding dynamic joltc...")
find_library(JOLTC_LIB NAMES joltc PATHS ${PREBUILT_LIB_PATH})
add_library(JOLTC_LIB SHARED IMPORTED)
message(STATUS "Linking against dynamic joltc lib")
target_link_libraries(${TARGET_NAME} PUBLIC 
    ${JOLTC_LIB}
)

# Reference "${JOLT_BINDINGS_ROOT}/PbClasses.cmake", kindly note that this test project is NOT shipped, so there's no need to copy any Google Abseil dynamic libraries even if (!USE_STATIC_PB).
set(absl_DIR $ENV{ABSEIL_INSTALL_DIR}/lib/cmake/absl)

set(utf8_range_DIR $ENV{PB_INSTALL_DIR}/lib/cmake/utf8_range)
find_package(utf8_range CONFIG REQUIRED)

set(Protobuf_DIR $ENV{PB_INSTALL_DIR}/lib/cmake/protobuf)
set(Protobuf_LIBRARY_PATH $ENV{PB_INSTALL_DIR}/lib)
set(Protobuf_INCLUDE_DIR $ENV{PB_INSTALL_DIR}/include)
set(Protobuf_PROTOC_EXECUTABLE $ENV{PB_INSTALL_DIR}/bin/protoc)
find_package(Protobuf CONFIG REQUIRED)

if(USE_STATIC_PB)
    target_link_libraries(${TARGET_NAME} PRIVATE 
            protobuf::libprotobuf
    )
else()
    target_link_libraries(${TARGET_NAME} PUBLIC 
            utf8_range::utf8_validity
    )
    target_link_libraries(${TARGET_NAME} PUBLIC 
            protobuf::libprotobuf
    )
endif()

set(ORIG_EXE_RPATH ${TARGET_NAME} INSTALL_RPATH)
set(NEW_EXE_RPATH "$ORIGIN")

if (ORIG_EXE_RPATH)
    set(EFF_EXE_RPATH "${ORIG_EXE_RPATH};${NEW_EXE_RPATH}")
else()
    set(EFF_EXE_RPATH "${NEW_EXE_RPATH}")
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    INSTALL_RPATH ${EFF_EXE_RPATH} 
) # Use "objdump -x /path/to/file | grep 'R.*PATH'" to verify

# ---------------------------- Installation ----------------------------
set(OVERRIDE_BINARY_DESTINATION ${CMAKE_BINARY_DIR}/${BUILD_TYPE}) 

if (MSVC)
else ()
    set_target_properties(
        ${TARGET_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
        LIBRARY_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
        ARCHIVE_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
    )
endif ()

set(MY_RUNTIME_DEPS_DESTINATIONS "${OVERRIDE_BINARY_DESTINATION}")

foreach (_rt_deps_destination ${MY_RUNTIME_DEPS_DESTINATIONS}) 
    if(MSVC)
        install (FILES ${PREBUILT_LIB_PATH}/joltc.dll
            DESTINATION ${_rt_deps_destination} COMPONENT Dependencies
        )
    else()
        install (FILES ${PREBUILT_LIB_PATH}/libjoltc.so
            DESTINATION ${_rt_deps_destination} COMPONENT Dependencies
        )
    endif()
    
    if (USE_STATIC_PB) 
    else()
        install(IMPORTED_RUNTIME_ARTIFACTS utf8_range::utf8_validity  
            DESTINATION ${_rt_deps_destination} COMPONENT Dependencies
        )
        install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf  
            DESTINATION ${_rt_deps_destination} COMPONENT Dependencies
        )
    endif()

    install(FILES ${PREBUILT_LIB_PATH}/PrimitiveConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
    install(FILES ${PREBUILT_LIB_PATH}/ConfigConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)

    install(FILES ${JOLT_BINDINGS_ROOT}/joltc/TestHelper.h DESTINATION ${_rt_deps_destination} OPTIONAL)
endforeach()
