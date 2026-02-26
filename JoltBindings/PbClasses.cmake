#[[

Assuming that the static-lib version of "protoc" is installed by either 
- "vcpkg install protobuf:x64-windows" (I use protobuf:x64-windows@6.33.4 for arena-allocation), or
- cmake build from source.

As an example, here're my ENV vars for [protobuf:x64-windows@6.33.4](https://vcpkg.io/en/package/protobuf.html) installed by vcpkg
- VCPKG_INSTALLED = C:/Users/yflu/vcpkg/installed/x64-windows
    - containing "protobuf-config.cmake" for both `dynamic linking` or `static linking/bundling`, and the whole vcpkg package is built by `Dynamic CRT`. See `<proj-root>/README.md` for more details.

]]


if (MSVC)
    if(NOT DEFINED ENV{VCPKG_INSTALLED})
        message(FATAL_ERROR "REQUIRED environment variable VCPKG_INSTALLED is not set!")
    endif()
    set(Protobuf_DIR "$ENV{VCPKG_INSTALLED}/share/protobuf")
    set(Protobuf_LIBRARY_PATH "$ENV{VCPKG_INSTALLED}/lib")
    set(Protobuf_INCLUDE_DIR "$ENV{VCPKG_INSTALLED}/include")
    set(Protobuf_PROTOC_EXECUTABLE "$ENV{VCPKG_INSTALLED}/tools/protobuf/protoc")
    set(absl_DIR "$ENV{VCPKG_INSTALLED}/share/absl")
    set(abseil_DIR "$ENV{VCPKG_INSTALLED}/share/abseil")
    set(utf8_range_DIR "$ENV{VCPKG_INSTALLED}/share/utf8_range")
    if(BUILD_TYPE STREQUAL "Debug")
        set(absl_DLL_DIR $ENV{VCPKG_INSTALLED}/debug/bin)
    else()
        set(absl_DLL_DIR $ENV{VCPKG_INSTALLED}/bin)
    endif()
endif()
find_package(Protobuf CONFIG REQUIRED)

if (USE_STATIC_PB)
    target_link_libraries(${TARGET_NAME} PRIVATE 
        protobuf::libprotobuf 
    )
else ()
    target_link_libraries(${TARGET_NAME} PUBLIC 
        protobuf::libprotobuf 
    )
endif ()

protobuf_generate(
    TARGET ${TARGET_NAME}
    EXPORT_MACRO JOLTC_EXPORT 
    LANGUAGE cpp
    GENERATE_EXTENSIONS .pb.h .pb.cc
    PROTOS serializable_data.proto
    PROTOC_OUT_DIR ${PB_GEN_ROOT}
)
