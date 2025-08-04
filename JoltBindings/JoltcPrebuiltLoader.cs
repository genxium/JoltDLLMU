using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace JoltcPrebuilt
{
    public static class Loader
    {
        public const string JOLT_LIB = "joltc";
        public const string PROTOBUF_LIB = "libprotobuf";
     
        [DllImport("kernel32", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi, EntryPoint = "LoadLibrary")]
        private static extern IntPtr LoadLibraryWindows(string path);

        [DllImport("libc", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi, EntryPoint = "dlopen")]
        private static extern IntPtr LoadLibraryLinux(string path, int flags);

        [DllImport("libdl", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi, EntryPoint = "dlopen")]
        private static extern IntPtr LoadLibraryMacOS(string path, int flags);

        private static IntPtr protobufLibptr;
        private static IntPtr libptr;

        public static bool LoadLibrary() {
            return LoadLibrary("");
        }

        public static bool LoadLibrary(string cpuArchName)
        {
            if (libptr != IntPtr.Zero)
            {
                return true;
            }

            var assemblyConfigurationAttribute = typeof(Loader).Assembly.GetCustomAttribute<AssemblyConfigurationAttribute>();
            var buildConfigurationName = assemblyConfigurationAttribute?.Configuration;
            bool isDebug = "Debug".Equals(buildConfigurationName); 

            string protobufLibname;
            string libname;

            if (IsWindows()) {
                protobufLibname = $"{PROTOBUF_LIB}{(isDebug ? "d" : "")}.dll";
                libname = $"{JOLT_LIB}.dll";
            } else if (IsLinux()) {
                protobufLibname = $"{PROTOBUF_LIB}.so";
                libname = $"lib{JOLT_LIB}.so";
            } else if (IsMacOS()) {
                protobufLibname = $"{PROTOBUF_LIB}{(isDebug ? "d" : "")}.dylib";
                libname = $"lib{JOLT_LIB}.dylib";
            } else {
                throw new Exception("Unrecognized platform, unable to load native lib.");
            }

            string assemblyFolder = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            string assemblySubFolder = "Plugins";
            if (assemblyFolder.EndsWith("Managed")) {
                // Packaged Unity build
                assemblyFolder = Directory.GetParent(assemblyFolder).ToString();
                if (!String.IsNullOrEmpty(cpuArchName)) {
                    assemblySubFolder = Path.Combine(assemblySubFolder, cpuArchName).ToString();
                }
            }
            bool pbres = TryLoadLibrary(Path.Combine(assemblyFolder, assemblySubFolder, protobufLibname), out protobufLibptr);
            if (protobufLibptr == IntPtr.Zero) {
                throw new Exception($"Failed to load {protobufLibname} from assemblyFolder = {assemblyFolder}, assemblySubFolder = {assemblySubFolder}");
            }
            bool res = TryLoadLibrary(Path.Combine(assemblyFolder, assemblySubFolder, libname), out libptr);
            if (libptr == IntPtr.Zero) {
                throw new Exception($"Failed to load {libname} from assemblyFolder = {assemblyFolder}, assemblySubFolder = {assemblySubFolder}");
            }
            return res;
        }

        private static bool TryLoadLibrary(string path, out IntPtr handle)
        {
            handle = IntPtr.Zero;

            if (IsWindows())
            {
                handle = LoadLibraryWindows(path);
            }
            else if (IsLinux())
            {
                handle = LoadLibraryLinux(path, 0x101);
            }
            else if (IsMacOS())
            {
                handle = LoadLibraryMacOS(path, 0x101);
            }

            return handle != IntPtr.Zero;
        }

        private static bool IsWindows()
        {
            return RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
        }

        private static bool IsLinux()
        {
            return RuntimeInformation.IsOSPlatform(OSPlatform.Linux);
        }

        private static bool IsMacOS()
        {
            return RuntimeInformation.IsOSPlatform(OSPlatform.OSX);
        }
    }
}
