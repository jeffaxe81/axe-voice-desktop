# Windows acceptance fixture: inject real kernel-object lookup failures into
# the production Inno installer and uninstaller. Requires an installed candidate.
function Invoke-NativeSetupErrorChecks {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Installer,
        [Parameter(Mandatory)][string]$Target,
        [Parameter(Mandatory)][string]$ApplicationMutexName,
        [Parameter(Mandatory)][string]$SetupMutexName
    )
    $ErrorActionPreference = 'Stop'
    if (!('NativeSetupErrorWindows' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
public static class NativeSetupErrorWindows {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern IntPtr CreateEventW(IntPtr attributes, [MarshalAs(UnmanagedType.Bool)] bool manualReset,
        [MarshalAs(UnmanagedType.Bool)] bool initialState, string name);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern IntPtr CreateMutexW(IntPtr attributes, [MarshalAs(UnmanagedType.Bool)] bool initialOwner, string name);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern IntPtr OpenMutexW(uint access, [MarshalAs(UnmanagedType.Bool)] bool inherit, string name);
    [DllImport("advapi32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool ConvertStringSecurityDescriptorToSecurityDescriptorW(string sddl, uint revision,
        out IntPtr descriptor, out uint size);
    [DllImport("advapi32.dll", SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetKernelObjectSecurity(IntPtr handle, uint information, IntPtr descriptor);
    [DllImport("kernel32.dll")] private static extern IntPtr LocalFree(IntPtr memory);

    // SYNCHRONIZE matches the production Inno lookup. Capture last error before
    // closing any handle so the injected failure has an independently known cause.
    public static int Probe(string name) {
        IntPtr raw = OpenMutexW(0x00100000, false, name);
        int error = Marshal.GetLastWin32Error();
        if (raw == IntPtr.Zero) return error;
        using (var handle = new SafeWaitHandle(raw, true)) { return 0; }
    }
    public static SafeWaitHandle CreateBlocker(string name, bool deniedMutex) {
        IntPtr raw = deniedMutex ? CreateMutexW(IntPtr.Zero, false, name)
                                : CreateEventW(IntPtr.Zero, true, false, name);
        int error = Marshal.GetLastWin32Error();
        if (raw == IntPtr.Zero) throw new Win32Exception(error);
        var handle = new SafeWaitHandle(raw, true);
        try {
            if (error == 183) throw new InvalidOperationException("Fixture object already exists: " + name);
            if (deniedMutex) {
                IntPtr descriptor = IntPtr.Zero;
                try {
                    uint size;
                    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW("D:P(D;;GA;;;WD)", 1, out descriptor, out size))
                        throw new Win32Exception(Marshal.GetLastWin32Error());
                    // The original CreateMutex full-access handle remains usable
                    // for closing even after the new DACL denies subsequent opens.
                    if (!SetKernelObjectSecurity(raw, 4, descriptor))
                        throw new Win32Exception(Marshal.GetLastWin32Error());
                } finally { if (descriptor != IntPtr.Zero) LocalFree(descriptor); }
            }
            return handle;
        } catch { handle.Dispose(); throw; }
    }
}
'@
    }
    $Target = (Resolve-Path -LiteralPath $Target).Path.TrimEnd('\')
    $Installer = (Resolve-Path -LiteralPath $Installer).Path
    $Uninstaller = Join-Path $Target 'unins000.exe'
    if (!(Test-Path -LiteralPath (Join-Path $Target 'AXE-Voice.exe') -PathType Leaf) -or
        !(Test-Path -LiteralPath $Uninstaller -PathType Leaf)) {
        throw 'Setup error fixture requires an installed candidate and its uninstaller'
    }
    function Assert-Absent([string]$Name) {
        $ErrorCode = [NativeSetupErrorWindows]::Probe($Name)
        if ($ErrorCode -ne 2) { throw "Expected absent marker '$Name'; OpenMutex returned $ErrorCode" }
    }
    function Get-PayloadSnapshot {
        (@(Get-ChildItem -LiteralPath $Target -Recurse -File -Force | Sort-Object FullName | ForEach-Object {
            "$($_.FullName.Substring($Target.Length))=$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash)"
        })) -join "`n"
    }
    function Invoke-SetupProcess([string]$Path, [string[]]$Arguments) {
        $Process = Start-Process -FilePath $Path -ArgumentList $Arguments -PassThru
        try {
            if (!$Process.WaitForExit(60000)) { throw "Production setup process timed out: $Path; no process was killed" }
            return $Process.ExitCode
        } finally { $Process.Dispose() }
    }
    $InstallArguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$Target`"")
    $UninstallArguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
    Assert-Absent $ApplicationMutexName
    Assert-Absent $SetupMutexName
    foreach ($Case in @(
        @{ Name = 'Event name collision'; Denied = $false; ErrorCode = 6 },
        @{ Name = 'Denied mutex access'; Denied = $true; ErrorCode = 5 }
    )) {
        $Blocker = [NativeSetupErrorWindows]::CreateBlocker($ApplicationMutexName, $Case.Denied)
        try {
            foreach ($Operation in @(
                @{ Name = 'installer'; Path = $Installer; Arguments = $InstallArguments },
                @{ Name = 'uninstaller'; Path = $Uninstaller; Arguments = $UninstallArguments }
            )) {
                $ActualError = [NativeSetupErrorWindows]::Probe($ApplicationMutexName)
                if ($ActualError -ne $Case.ErrorCode) {
                    throw "$($Case.Name): expected OpenMutex error $($Case.ErrorCode), got $ActualError"
                }
                $Before = Get-PayloadSnapshot
                $Code = Invoke-SetupProcess $Operation.Path $Operation.Arguments
                if ($Code -eq 0) { throw "$($Case.Name): production $($Operation.Name) accepted an unsafe operation" }
                if ((Get-PayloadSnapshot) -cne $Before) {
                    throw "$($Case.Name): rejected $($Operation.Name) changed the installed payload"
                }
                Assert-Absent $SetupMutexName
            }
        } finally { $Blocker.Dispose() }
        Assert-Absent $ApplicationMutexName
        Assert-Absent $SetupMutexName
        if ((Invoke-SetupProcess $Installer $InstallArguments) -ne 0) {
            throw "$($Case.Name): installer retry failed after the blocking object was closed"
        }
        Assert-Absent $SetupMutexName
    }
    Write-Output 'Production installer/uninstaller fail closed for ERROR_INVALID_HANDLE and ERROR_ACCESS_DENIED; payload preservation and installer retries passed'
}
