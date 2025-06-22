if (XCODE)
    # Ensure that we enable SSE4.2 for the x86_64 build, XCode builds multiple architectures
    set_property(TARGET ${TARGET_NAME} PROPERTY XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS[arch=x86_64] "$(inherited) -msse4.2 -mpopcnt")
endif()

if (MSVC)
    # Debug information
    target_compile_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Debug>:/Zi>)

    # Enable full optimization in dev/release
    target_compile_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Debug>:/Od> $<$<NOT:$<CONFIG:Debug>>:/Ox>)

    # Inline function expansion
    target_compile_options(${TARGET_NAME} PRIVATE /Ob2)

     # Enable intrinsic functions in dev/release
    target_compile_options(${TARGET_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Oi>)

    # Favor fast code
    target_compile_options(${TARGET_NAME} PRIVATE /Ot)

    # Enable fiber-safe optimizations in dev/release
    target_compile_options(${TARGET_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GT>)

    # Enable string pooling
    target_compile_options(${TARGET_NAME} PRIVATE /GF)

    # Use security checks only in debug
    target_compile_options(${TARGET_NAME} PRIVATE $<$<CONFIG:DEBUG>:/sdl> $<$<NOT:$<CONFIG:DEBUG>>:/sdl->)
else()

endif ()

target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}> # for generated header
    $<BUILD_INTERFACE:${PB_GEN_ROOT}> # For pb class headers
    $<BUILD_INTERFACE:${JOLTC_ROOT}>
    $<BUILD_INTERFACE:${JOLTC_ROOT}/include>
    $<INSTALL_INTERFACE:/include>)
