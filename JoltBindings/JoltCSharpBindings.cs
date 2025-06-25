using System;
using System.Runtime.InteropServices;

namespace JoltCSharp {
    public unsafe class Bindings {
        private const string JOLT_LIB = JoltcPrebuilt.Loader.JOLT_LIB;
        /* All defined in 'joltc.h'*/

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool PrimitiveConsts_Init(char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool ConfigConsts_Init(char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool JPH_Init(int nBytesForTempAllocator); // Creates default allocators

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool JPH_Shutdown(); // Destroys default allocators

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern UIntPtr APP_CreateBattle(char* inBytes, int inBytesCnt, [MarshalAs(UnmanagedType.U1)] bool isFrontend, [MarshalAs(UnmanagedType.U1)] bool isOnlineArenaMode);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_DestroyBattle(UIntPtr inBattle, [MarshalAs(UnmanagedType.U1)] bool isFrontend);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_Step(UIntPtr inBattle, int fromRdfId, int toRdfId, [MarshalAs(UnmanagedType.U1)] bool isChasing);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_GetRdf(UIntPtr inBattle, int inRdfId, char* outBytesPreallocatedStart, IntPtr outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_UpsertCmd(UIntPtr inBattle, int inIfdId, uint inSingleJoinIndex, ulong inSingleInput, char* outBytesPreallocatedStart, IntPtr outBytesCntLimit, [MarshalAs(UnmanagedType.U1)] bool fromUdp, [MarshalAs(UnmanagedType.U1)] bool fromTcp, [MarshalAs(UnmanagedType.U1)] bool isFrontend);

        //------------------------------------------------------------------------------------------------
        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl)]
        static extern void RegisterDebugCallback(debugCallback cb);
        delegate void debugCallback(IntPtr request, int color, int size);
        enum Color { red, green, blue, black, white, yellow, orange };

        /*
        // To be put on Unity side
        [MonoPInvokeCallback(typeof(debugCallback))]
        static void OnDebugCallback(IntPtr request, int color, int size) {
            // Ptr to string
            string rawMsg = Marshal.PtrToStringAnsi(request, size);

            // Add Specified Color
            var coloredMsg = $"<color={((Color)color).ToString()}>{rawMsg}</color>";
            Debug.Log(coloredMsg);
        }
        */
    }
}

