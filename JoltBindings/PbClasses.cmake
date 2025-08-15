#[[

Assuming that "protoc" is installed by either 
- "vcpkg install protobuf protobuf:x64-windows", or
- cmake build from source.

]]

if (WIN32)
    if(NOT DEFINED ENV{VCPKG_INSTALLED_SHARE})
        message(FATAL_ERROR "REQUIRED environment variable VCPKG_INSTALLED_SHARE is not set!")
    endif()
    set(protobuf_DIR "$ENV{VCPKG_INSTALLED_SHARE}/protobuf")
    find_package(protobuf CONFIG REQUIRED)
else()
    find_package(Protobuf CONFIG REQUIRED)
endif()

target_link_libraries(${TARGET_NAME} PUBLIC 
    protobuf::libprotobuf 
)

protobuf_generate(
    TARGET ${TARGET_NAME}
    EXPORT_MACRO JOLTC_EXPORT 
    LANGUAGE cpp
    GENERATE_EXTENSIONS .pb.h .pb.cc
    PROTOS serializable_data.proto
    PROTOC_OUT_DIR ${PB_GEN_ROOT}
)
