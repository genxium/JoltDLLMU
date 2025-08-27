set(RING_BUFFER_MT_TEST_ROOT ${JOLT_BINDINGS_ROOT}/CppTests/RingBufferMt)

# Source files
set(RING_BUFFER_MT_TEST_SRC_FILES
	${RING_BUFFER_MT_TEST_ROOT}/Test.cpp
    ${JOLT_BINDINGS_ROOT}/joltc/DebugLog.cpp
)

add_executable(RingBufferMtTest ${RING_BUFFER_MT_TEST_SRC_FILES})

set_target_properties(
    RingBufferMtTest
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
    LIBRARY_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
    ARCHIVE_OUTPUT_DIRECTORY "${OVERRIDE_BINARY_DESTINATION}"
)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RingBufferMtTest)
    target_link_options(RingBufferMtTest PUBLIC "/SUBSYSTEM:CONSOLE")

	# Enable use of exceptions in MSVC's STL
	target_compile_definitions(RingBufferMtTest PUBLIC $<$<BOOL:${MSVC}>:_HAS_EXCEPTIONS=1>)
	target_compile_options(RingBufferMtTest PRIVATE /EHsc)
endif()

target_include_directories(RingBufferMtTest PUBLIC
    $<BUILD_INTERFACE:${JOLT_BINDINGS_ROOT}/joltc>
    $<BUILD_INTERFACE:${PB_GEN_ROOT}>
    $<INSTALL_INTERFACE:/include>)
