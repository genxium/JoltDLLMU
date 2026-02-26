# Does this lib actually work?

Yes, there's a closed-source project dedicated for the account system, backend session management and frontend rendering, here's [a screen-recording of a battle over the internet (ping around 20ms~300ms during test)](https://pan.baidu.com/s/1SdXJFFyo0_z0G8yhuavpKQ?pwd=43jb).

![latest_demo](./Screenshots/DLLMUJolt_milestone_1.gif)

Afterall, the underlying netcode is the same as [DelayNoMoreUnity](https://github.com/genxium/DelayNoMoreUnity/tree/v2.3.4).

# Why NOT use a "static library `joltc`"?

The main target `joltc.dll` (or `libjoltc.so`) is used by `JoltCSharpBindings.cs` via `DllImport("joltc")` which doesn't support static library, so there's no need to support static library build in the cmake scripts.

- This requirement has a profound impact on our choice of `C Runtime Library (CRT)` on Windows, when installing `protobuf` by `vcpkg`, we can only use `protobuf:x64-windows`(built with `Dynamic CRT`) but NOT `protobuf:x64-windows-static` (built with `Static CRT`).

- Kindly note that `protobuf:x64-windows` can also provide `libprotobuf.lib` which is to be `statically linked/bundled`, and the main constraint for this choice is that **eventually `joltc.dll` requires other `Dynamic CRT built/compatible DLLs` from somewhere in the Windows file system to run even if `libprotobuf.lib` is already `statically linked/bundled`**.  

# Why use a "static library `protobuf`" by default?

First of all, it's [recommended by the library itself](https://github.com/protocolbuffers/protobuf/tree/v31.1/cmake#dlls-vs-static-linking).

Besides the reasons given above, it's also important to build [Google Abseil](https://protobuf.dev/reference/cpp/abseil/) as "static libraries" for `Protobuf v22+` to avoid cascade-dynamic-linking. 

What if Google Abseil is built as "dynamic libraries" instead? In that case [all these dependencies](https://github.com/protocolbuffers/protobuf/blob/v31.1/cmake/abseil-cpp.cmake#L56) have to be copied for shipping on Linux -- moreover, there're internal cascade-dynamic-linking within Google Abseil itself too (e.g. `absl::cord` requires a few `libabsl_*_internal_*.so` files). That said, I did take the challenge and succeeded in building a shippable `libjoltc.so` by `<proj-root>/start_linux_dynamic_pb_docker_container_interactive.sh` (e.g. verified by a shipped C# backend service).
