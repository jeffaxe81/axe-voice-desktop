using System;
using System.Runtime.InteropServices;

namespace AXEVoice.WinUI3;

// Read-only ABI. No SIP credentials, configure/register/dial calls or secrets.
// The bridge deliberately reports no native backend attached in this phase.
internal static class NativeBridge
{
    internal const uint SupportedAbi = 2;

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

    internal enum CommandResult
    {
        Accepted=0, Invalid=-1, Unavailable=-2, Rejected=-3, MissingAccount=-4
    }

    [DllImport("axv_frontend_bridge.dll",
        EntryPoint="axv_frontend_connect_saved",CallingConvention=CallingConvention.Cdecl)]
    private static extern int ConnectSavedNative();

    [DllImport("axv_frontend_bridge.dll",
        EntryPoint="axv_frontend_disconnect",CallingConvention=CallingConvention.Cdecl)]
    private static extern int DisconnectNative();

    [DllImport("axv_frontend_bridge.dll",
        EntryPoint="axv_frontend_dial",CallingConvention=CallingConvention.Cdecl,
        CharSet=CharSet.Unicode)]
    private static extern int DialNative(
        [MarshalAs(UnmanagedType.LPWStr)] string destination,uint length,
        ulong expectedSession);

    [DllImport("axv_frontend_bridge.dll",
        EntryPoint="axv_frontend_answer",CallingConvention=CallingConvention.Cdecl)]
    private static extern int AnswerNative(ulong session,ulong callId);

    [DllImport("axv_frontend_bridge.dll",
        EntryPoint="axv_frontend_hangup",CallingConvention=CallingConvention.Cdecl)]
    private static extern int HangupNative(ulong session,ulong callId);

    private static CommandResult Invoke(Func<int> call)
    {
        try {
            if(GetVersion()!=SupportedAbi)return CommandResult.Unavailable;
            return (CommandResult)call();
        }
        catch(DllNotFoundException){return CommandResult.Unavailable;}
        catch(EntryPointNotFoundException){return CommandResult.Unavailable;}
        catch(BadImageFormatException){return CommandResult.Unavailable;}
    }
    internal static CommandResult ConnectSaved()=>Invoke(ConnectSavedNative);
    internal static CommandResult Disconnect()=>Invoke(DisconnectNative);
    internal static CommandResult Dial(string destination,ulong expectedSession)
        =>Invoke(()=>DialNative(destination,(uint)destination.Length,expectedSession));
    internal static CommandResult Answer(ulong session,ulong callId)
        =>Invoke(()=>AnswerNative(session,callId));
    internal static CommandResult Hangup(ulong session,ulong callId)
        =>Invoke(()=>HangupNative(session,callId));

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
