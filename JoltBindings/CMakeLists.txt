cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(JoltBindings VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

set(BUILD_SHARED_LIBS ON)

set(JOLT_BINDINGS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PHYSICS_REPO_ROOT ${JOLT_BINDINGS_ROOT}/../joltphysics_v5.3.0)
set(JOLTC_ROOT ${JOLT_BINDINGS_ROOT}/joltc)
set(PB_GEN_ROOT $ENV{PB_GEN_ROOT})

set(TARGET_NAME joltc)

include(${JOLT_BINDINGS_ROOT}/JoltBindingsBeforeTarget.cmake)

if (BUILD_SHARED_LIBS)
	add_library(${TARGET_NAME} SHARED ${JOLTC_SRC_FILES})

    #Define macro named "JOLTC_EXPORT" for "PbClasses.cmake".
    include(GenerateExportHeader)
    generate_export_header(${TARGET_NAME}
        EXPORT_MACRO_NAME JOLTC_EXPORT
        EXPORT_FILE_NAME joltc_export.h
    ) # The file is supposed to be in "${CMAKE_CURRNET_BINARY_DIR}"
    
	target_compile_definitions(${TARGET_NAME} PUBLIC JPH_SHARED_LIBRARY)
	target_compile_definitions(${TARGET_NAME} PRIVATE JPH_BUILD_SHARED_LIBRARY)
    if (WIN32)
	    target_compile_definitions(${TARGET_NAME} PRIVATE JPH_PLATFORM_WINDOWS)
    endif()

    # [IMPORTANT] Force "joltc_export.h" to be included at the beginning of every source file compiled, even if it's not explicitly included in the source code -- mainly serves "serializable_data.pb.h to recognize JOLTC_EXPORT" which needs this injection due to being automatically generated.
    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /FI$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/joltc_export.h)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -include $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/joltc_export.h)
    endif()
else()
	add_library(${TARGET_NAME} ${JOLTC_SRC_FILES})
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

install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}
    ARCHIVE DESTINATION ${OVERRIDE_INSTALL_DESTINATION} # Not needed  
    LIBRARY DESTINATION ${OVERRIDE_INSTALL_DESTINATION} 
    RUNTIME DESTINATION ${OVERRIDE_INSTALL_DESTINATION}
)

install(FILES 
    # Regular headers below
	${JOLTC_ROOT}/include/CollisionLayers.h
	${JOLTC_ROOT}/include/CollisionCallbacks.h
	${JOLTC_ROOT}/include/BackendBattle.h
	${JOLTC_ROOT}/include/BaseBattle.h
	${JOLTC_ROOT}/include/BattleConsts.h
	${JOLTC_ROOT}/include/FrameRingBuffer.h
	${JOLTC_ROOT}/include/FrontendBattle.h
    ${JOLTC_ROOT}/include/joltc_api.h 
    ${JOLTC_ROOT}/include/DebugLog.h

    # Special headers below
	${PB_GEN_ROOT}/serializable_data.pb.h 
    
    #DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    DESTINATION ${OVERRIDE_INSTALL_DESTINATION}/include # For convenience
)

install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf 
    #DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Dependencies
    DESTINATION ${OVERRIDE_INSTALL_DESTINATION} COMPONENT Dependencies # For convenience
)

if (BUILD_SHARED_LIBS)
	message(STATUS "  Library         SHARED")
else ()
  	message(STATUS "  Library         STATIC")
endif ()

if (CMAKE_GENERATOR_PLATFORM)
    message(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
endif()

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_OSX_ARCHITECTURES)
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()
