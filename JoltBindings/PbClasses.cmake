set(absl_DIR $ENV{ABSEIL_INSTALL_DIR}/lib/cmake/absl)
find_package(absl CONFIG REQUIRED)

set(utf8_range_DIR $ENV{PB_INSTALL_DIR}/lib/cmake/utf8_range)
find_package(utf8_range CONFIG REQUIRED)

set(Protobuf_DIR $ENV{PB_INSTALL_DIR}/lib/cmake/protobuf)
set(Protobuf_LIBRARY_PATH $ENV{PB_INSTALL_DIR}/lib)
set(Protobuf_INCLUDE_DIR $ENV{PB_INSTALL_DIR}/include)
set(Protobuf_PROTOC_EXECUTABLE $ENV{PB_INSTALL_DIR}/bin/protoc)
find_package(Protobuf CONFIG REQUIRED)

if (USE_STATIC_PB)
    target_link_libraries(${TARGET_NAME} PRIVATE 
        protobuf::libprotobuf 
    )
else ()
    target_link_libraries(${TARGET_NAME} PUBLIC 
        utf8_range::utf8_validity
    )
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
