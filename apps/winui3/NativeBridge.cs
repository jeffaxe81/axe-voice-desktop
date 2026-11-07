using System;
using System.Runtime.InteropServices;

namespace AXEVoice.WinUI3;

// Read-only ABI. No SIP credentials, configure/register/dial calls or secrets.
// The bridge deliberately reports no native backend attached in this phase.
internal static class NativeBridge
{
    internal const uint SupportedAbi = 1;

    [StructLayout(LayoutKind.Sequential)]
    internal struct Snapshot
    {
        public uint Size;
        public uint AbiVersion;
        public uint BackendAttached;
        public uint Connection;
        public uint View;
        public uint CanDial;
        public uint CanAnswer;
        public uint CanHangup;
        public uint CanSecondary;
        public ulong Session;
        public ulong CallId;
    }

    [DllImport("axv_frontend_bridge.dll", EntryPoint = "axv_frontend_abi_version",
        CallingConvention = CallingConvention.Cdecl)]
    private static extern uint GetVersion();

    [DllImport("axv_frontend_bridge.dll", EntryPoint = "axv_frontend_get_snapshot",
        CallingConvention = CallingConvention.Cdecl)]
    private static extern int GetSnapshot(out Snapshot snapshot, uint size);

    internal enum DestinationStatus { Valid=0, Empty=1, Invalid=2, Unavailable=3 }

    [DllImport("axv_frontend_bridge.dll",
        EntryPoint = "axv_frontend_validate_destination",
        CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ValidateDestination(
        [MarshalAs(UnmanagedType.LPWStr)] string destination,uint length);

    internal static DestinationStatus Validate(string destination)
    {
        if(destination.Length>128)return DestinationStatus.Invalid;
        try {
            int code=ValidateDestination(destination,(uint)destination.Length);
            return code switch {
                0=>DestinationStatus.Valid,
                1=>DestinationStatus.Empty,
                2=>DestinationStatus.Invalid,
                _=>DestinationStatus.Unavailable
            };
        }
        catch (DllNotFoundException) { return DestinationStatus.Unavailable; }
        catch (EntryPointNotFoundException) { return DestinationStatus.Unavailable; }
        catch (BadImageFormatException) { return DestinationStatus.Unavailable; }
    }

    internal static bool TryRead(out Snapshot snapshot)
    {
        snapshot = default;
        try
        {
            if (GetVersion() != SupportedAbi) return false;
            uint size = checked((uint)Marshal.SizeOf<Snapshot>());
            if (GetSnapshot(out snapshot, size) != 0) return false;
            return snapshot.Size == size && snapshot.AbiVersion == SupportedAbi;
        }
        catch (DllNotFoundException) { return false; }
        catch (EntryPointNotFoundException) { return false; }
        catch (BadImageFormatException) { return false; }
    }
}
