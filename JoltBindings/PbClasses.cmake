# [WARNING] Assuming that "protoc" is installed by "vcpkg install protobuf protobuf:x64-windows".
set(protobuf_DIR "$ENV{VCPKG_INSTALLED_SHARE}/protobuf")
find_package(protobuf CONFIG REQUIRED)

target_link_libraries(${TARGET_NAME} PUBLIC 
            protobuf::libprotobuf
            protobuf::libprotoc # [WARNING] Only needed during compilation, for runtime dependency only "install(IMPORTED_RUNTIME_ARTIFACTS protobuf::libprotobuf)" 
)

protobuf_generate(
    TARGET ${TARGET_NAME}
    EXPORT_MACRO JOLTC_EXPORT 
    LANGUAGE cpp
    GENERATE_EXTENSIONS .pb.h .pb.cc
    PROTOS serializable_data.proto
    PROTOC_OUT_DIR ${PB_GEN_ROOT}
)
#[[ On Windows, verify that pb classes are successfully exported by 
```
dumpbin /exports \path\to\joltc.dll | grep -i "CharacterDownsync" --color
```
]]
