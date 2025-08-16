# Why not use a "static library `joltc`"?

The main target `joltc.dll` (or `libjoltc.so`) is used by `JoltCSharpBindings.cs` via `DllImport("joltc")` which doesn't support static library, so there's no need to support static library build in the cmake scripts.

# Why use a "static library `protobuf`"?

First of all, it's [recommended by the library itself](https://github.com/protocolbuffers/protobuf/tree/v31.1/cmake#dlls-vs-static-linking).

I guess that for "protobuf v22+" which requires [Google Abseil as a dependency](https://protobuf.dev/reference/cpp/abseil/) (by dynamic-linking), using a "static library protobuf" can reduce cascade-dynamic-linking to some extend.
