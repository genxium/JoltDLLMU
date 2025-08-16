# Why not use a "static library `joltc`"?

The main target `joltc.dll` (or `libjoltc.so`) is used by `JoltCSharpBindings.cs` via `DllImport("joltc")` which doesn't support static library, so there's no need to support static library build in the cmake scripts.

# Why use a "static library `protobuf`" by default?

First of all, it's [recommended by the library itself](https://github.com/protocolbuffers/protobuf/tree/v31.1/cmake#dlls-vs-static-linking).

Besides the reasons given above, it's also important to build [Google Abseil](https://protobuf.dev/reference/cpp/abseil/) as "static libraries" for `Protobuf v22+` to avoid cascade-dynamic-linking. 

What if Google Abseil is built as "dynamic libraries" instead? In that case [all these dependencies](https://github.com/protocolbuffers/protobuf/blob/v31.1/cmake/abseil-cpp.cmake#L56) have to be copied for shipping on Linux -- moreover, there're internal cascade-dynamic-linking within Google Abseil itself too (e.g. `absl::cord` requires a few `libabsl_*_internal_*.so` files). That said, I did take the challenge and succeeded in building a shippable `libjoltc.so` by `<proj-root>/start_linux_dynamic_pb_docker_container_interactive.sh` (e.g. verified by a shipped C# backend service).
