cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(JoltBindings VERSION 0.0.1 LANGUAGES CXX)

# Requires C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()

include(GNUInstallDirs)

if (WIN32)
else()
    set(CMAKE_CXX_COMPILER "/usr/bin/c++")
endif()

set(JOLT_BINDINGS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PHYSICS_REPO_ROOT ${JOLT_BINDINGS_ROOT}/../joltphysics_v5.3.0)
set(JOLTC_ROOT ${JOLT_BINDINGS_ROOT}/joltc)
if ($ENV{USE_STATIC_PB} MATCHES "true") 
    set(USE_STATIC_PB ON)
endif()

set(TARGET_NAME joltc)
SET(CMAKE_INSTALL_RPATH "$ORIGIN") # Use "objdump -x /path/to/file | grep 'R.*PATH'" to verify 

SET(BUILD_TYPE $ENV{BUILD_TYPE})
message(STATUS "JoltBindings/CMakeLists.txt, build type=${BUILD_TYPE}")

set(OVERRIDE_BINARY_DESTINATION ${CMAKE_BINARY_DIR}/${BUILD_TYPE}) 

set(PB_GEN_ROOT ${OVERRIDE_BINARY_DESTINATION}/pb_gen)

include(${JOLT_BINDINGS_ROOT}/JoltBindingsBeforeTarget.cmake)

add_library(${TARGET_NAME} SHARED ${JOLTC_SRC_FILES})

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

target_compile_definitions(${TARGET_NAME} PRIVATE JPH_SHARED_LIBRARY)
target_compile_definitions(${TARGET_NAME} PRIVATE JPH_BUILD_SHARED_LIBRARY)
if (WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE JPH_PLATFORM_WINDOWS)
endif()

# [IMPORTANT] Force "joltc_export.h" to be included at the beginning of every source file compiled, even if it's not explicitly included in the source code -- mainly serves "serializable_data.pb.h to recognize JOLTC_EXPORT" which needs this injection due to being automatically generated.

# Define macro named "JOLTC_EXPORT" for "PbClasses.cmake". 
include(GenerateExportHeader)
generate_export_header(${TARGET_NAME}
    EXPORT_MACRO_NAME JOLTC_EXPORT
    EXPORT_FILE_NAME "${PB_GEN_ROOT}/joltc_export.h" # By default, this command generates a header file named <target-name-lowercase>_export.h in the current binary directory (CMAKE_CURRENT_BINARY_DIR), see https://cmake.org/cmake/help/latest/module/GenerateExportHeader.html.
)

if(MSVC)
    target_compile_definitions(${TARGET_NAME} PRIVATE joltc_EXPORTS) # [IMPORTANT] Moreover, ${TARGET_NAME} should toggle on "joltc_EXPORTS", a weird flag used in the autogen "joltc_export.h" to enable "__declspec(dllexport)" in Windows.
    target_compile_options(${TARGET_NAME} PRIVATE /FI$<BUILD_INTERFACE:${PB_GEN_ROOT}>/joltc_export.h)
    # Expand inline functions for easier debugging.
    target_compile_options(${TARGET_NAME} PUBLIC
        $<$<CONFIG:Debug>: /FI$<BUILD_INTERFACE:${JOLTC_ROOT}>/debug_prelude.h>
    )
else()
    # [IMPORTANT] No need to toggle on the weird flag "joltc_EXPORTS" for Linux build.
    target_compile_options(${TARGET_NAME} PRIVATE -include $<BUILD_INTERFACE:${PB_GEN_ROOT}>/joltc_export.h)
    target_compile_options(${TARGET_NAME} PUBLIC
        $<$<CONFIG:Debug>: -include $<BUILD_INTERFACE:${JOLTC_ROOT}>/debug_prelude.h>
    )
endif()
    
#target_compile_definitions(${TARGET_NAME} PUBLIC USE_RBMT)

# Set the debug/non-debug build flags
target_compile_definitions(${TARGET_NAME} PUBLIC "$<$<CONFIG:Release,Distribution,ReleaseASAN,ReleaseUBSAN,ReleaseTSAN,ReleaseCoverage>:NDEBUG>")
if(BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Turning on JPH_ASSERT for BUILD_TYPE=Debug")
else()
    message(STATUS "Turning off JPH_ASSERT for BUILD_TYPE=Release")
    target_compile_definitions(${TARGET_NAME} PUBLIC JPH_NO_DEBUG)
    target_compile_definitions(${TARGET_NAME} PUBLIC NDEBUG)
endif()

include(${PHYSICS_REPO_ROOT}/Jolt/Jolt.cmake)
target_sources(${TARGET_NAME} PRIVATE ${JOLT_PHYSICS_SRC_FILES})

include(${JOLT_BINDINGS_ROOT}/PbClasses.cmake)
target_sources(${TARGET_NAME} PRIVATE ${PB_GEN_ROOT}/serializable_data.pb.cc)

include(${JOLT_BINDINGS_ROOT}/JoltBindingsAfterTarget.cmake)

# ---------------------------- Installation ----------------------------
#[[

1. When importing by downstream C# application, only "joltc.dll" is enough to provide all runtime interfacing features, i.e. to call the exported functions in "joltc_api.h". There's no way a downstream C# application can make use of our header files or the exported symbols in "joltc.lib" for compiling awareness.

2. When importing by downstream C++ application, it's certainly possible to just call the exported functions in "joltc_api.h" by linking "joltc.dll" alone -- however by adding "joltc.lib" and our header files for compiling, it can make full use of all exported symbols (i.e. functions and classes).

]]
set(OVERRIDE_INSTALL_DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../UnityPackageOutput/Runtime/Plugins) 

if (USE_STATIC_PB) 
    message(STATUS "Static protobuf libs are embedded")
    install(TARGETS ${TARGET_NAME} 
        EXPORT ${TARGET_NAME}
        ARCHIVE DESTINATION ${OVERRIDE_INSTALL_DESTINATION}   
        LIBRARY DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
        RUNTIME DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
    )
else()
    if (MSVC)
        install(TARGETS ${TARGET_NAME} 
            EXPORT ${TARGET_NAME}
            ARCHIVE DESTINATION ${OVERRIDE_INSTALL_DESTINATION}   
            LIBRARY DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
            RUNTIME DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
        )
    else ()
        install(TARGETS ${TARGET_NAME} 
            EXPORT ${TARGET_NAME}
            RUNTIME_DEPENDENCY_SET joltc_rt_deps 
            ARCHIVE DESTINATION ${OVERRIDE_INSTALL_DESTINATION}  
            LIBRARY DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
            RUNTIME DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
        ) 

        # [REMINDER] Don't forget to verify that ALL required "Google Abseil dynamic libraries" are copied by "ldd ${OVERRIDE_INSTALL_DESTINATION}/libjoltc.so | grep libabsl --color".
        install(RUNTIME_DEPENDENCY_SET joltc_rt_deps 
            POST_INCLUDE_REGEXES "(.*)libabsl(.*)"
            POST_EXCLUDE_REGEXES "(.*)ld-(.*)" "(.*)libc(.*)" "(.*)libgcc(.*)" "(.*)libm(.*)" "(.*)librt(.*)" "(.*)libstdc(.*)" "(.*)libpthread(.*)" "(.*)linux-vdso(.*)"  
            LIBRARY DESTINATION ${OVERRIDE_INSTALL_DESTINATION} COMPONENT Dependencies 
            RUNTIME DESTINATION ${OVERRIDE_INSTALL_DESTINATION} COMPONENT Dependencies
        ) 
    endif()

    message(STATUS "Copying protobuf runtime deps")
    install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf  
        DESTINATION ${OVERRIDE_INSTALL_DESTINATION} COMPONENT Dependencies
    )
endif()

# Regular headers below
install(DIRECTORY 
    ${JOLTC_ROOT}/

    #DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    DESTINATION ${OVERRIDE_INSTALL_DESTINATION}/include # For convenience

    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.inl"
)

install(DIRECTORY  
    ${PHYSICS_REPO_ROOT}/ 
    
    DESTINATION ${OVERRIDE_INSTALL_DESTINATION}/include 
    
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.inl"
)

# Special headers below
install(FILES 
    $<BUILD_INTERFACE:${PB_GEN_ROOT}>/serializable_data.pb.h 
    $<BUILD_INTERFACE:${PB_GEN_ROOT}>/joltc_export.h
    
    DESTINATION ${OVERRIDE_INSTALL_DESTINATION}/include # For convenience
)

if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${OVERRIDE_INSTALL_DESTINATION} OPTIONAL)
endif()

if (CMAKE_GENERATOR_PLATFORM)
    message(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
endif()

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_OSX_ARCHITECTURES)
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()

set(TARGET_BACKEND_TEST ON)
if (TARGET_BACKEND_TEST) 
    include(${JOLT_BINDINGS_ROOT}/CppTests/Backend/Backend.cmake)
endif()

set(TARGET_FRONTEND_TEST ON)
if (TARGET_FRONTEND_TEST) 
    include(${JOLT_BINDINGS_ROOT}/CppTests/Frontend/Frontend.cmake)
endif()

set(TARGET_RBMT_TEST ON)
if (TARGET_RBMT_TEST) 
    include(${JOLT_BINDINGS_ROOT}/CppTests/RingBufferMt/RingBufferMt.cmake)
endif()
