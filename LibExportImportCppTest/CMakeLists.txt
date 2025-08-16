cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(LibExportImportCppTest VERSION 0.0.1 LANGUAGES CXX)

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

set(JOLT_BINDINGS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../JoltBindings)
set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../joltphysics_v5.3.0)
set(PREBUILT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../UnityPackageOutput/Runtime/Plugins)
set(PREBUILT_HEADER_PATH ${PREBUILT_LIB_PATH}/include)

set(TARGET_NAME LibExportImportCppTest)

set(CPP_TEST_SRC_FILES 
    Test.cpp
)

add_executable(${TARGET_NAME} ${CPP_TEST_SRC_FILES})

target_include_directories(${TARGET_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${PREBUILT_HEADER_PATH}>
	$<INSTALL_INTERFACE:/include>)

find_library(JOLTC_LIB NAMES joltc PATHS ${PREBUILT_LIB_PATH})
add_library(JOLTC_LIB SHARED IMPORTED)

# Reference "${JOLT_BINDINGS_ROOT}/PbClasses.cmake", kindly note that this test project is NOT shipped, so there's no need to copy any Google Abseil dynamic libraries even if (!USE_STATIC_PB).
if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
    if(NOT DEFINED ENV{VCPKG_INSTALLED_SHARE})
        message(FATAL_ERROR "REQUIRED environment variable VCPKG_INSTALLED_SHARE is not set!")
    endif()
    set(protobuf_DIR "$ENV{VCPKG_INSTALLED_SHARE}/protobuf")
    find_package(protobuf CONFIG REQUIRED)
else()
    find_package(Protobuf CONFIG REQUIRED)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE 
        ${JOLTC_LIB}
        protobuf::libprotobuf
)

if (MSVC)
else()
    # Tells the linker to add the directory where the executable resides ($ORIGIN) to its search path for shared libraries.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,'$ORIGIN'")
endif()

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

set(MY_RUNTIME_DEPS_DESTINATIONS "${CMAKE_INSTALL_BINDIR}")
if (MSVC) 
    list(APPEND MY_RUNTIME_DEPS_DESTINATIONS "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>") # This one is for Visual Studio to pick up runtime dependent DLLs
endif()

foreach (_rt_deps_destination ${MY_RUNTIME_DEPS_DESTINATIONS}) 
    install (FILES ${JOLTC_LIB}
        DESTINATION ${_rt_deps_destination} COMPONENT Dependencies # For convenience
    )
    
    if (USE_STATIC_PB) 
    else()
        install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf  
            DESTINATION ${OVERRIDE_INSTALL_DESTINATION} COMPONENT Dependencies
        )
    endif()

    install(FILES ${PREBUILT_LIB_PATH}/PrimitiveConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
    install(FILES ${PREBUILT_LIB_PATH}/ConfigConsts.pb DESTINATION ${_rt_deps_destination} OPTIONAL)
endforeach()
