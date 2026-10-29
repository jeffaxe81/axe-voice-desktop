# Windows acceptance fixture: exercise the production Inno wizard reservation.
# Dot-source this file after installing the candidate into the isolated Target.
function Invoke-NativeSetupContentionCheck {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Installer,
        [Parameter(Mandatory)][string]$Target,
        [Parameter(Mandatory)][string]$ApplicationMutexName,
        [Parameter(Mandatory)][string]$SetupMutexName,
        [int]$BusyExitCode = 170
    )
    $ErrorActionPreference = 'Stop'
    if (!( 'NativeSetupContentionWindows' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class NativeSetupContentionWindows {
    private delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr window, int id);
    public static IntPtr[] Windows(int pid) {
        var windows = new List<IntPtr>();
        EnumWindows((window, parameter) => {
            uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner == (uint)pid && IsWindowVisible(window)) windows.Add(window);
            return true;
        }, IntPtr.Zero);
        return windows.ToArray();
    }
}
'@
    }
    function Test-Marker([string]$Name) {
        try { $Handle = [Threading.Mutex]::OpenExisting($Name) }
        catch [Threading.WaitHandleCannotBeOpenedException] { return $false }
        # Existence protocol only. Close the probe immediately so it cannot
        # extend the production marker's lifetime after cancellation.
        try { return $true } finally { $Handle.Dispose() }
    }
    function Get-PayloadSnapshot {
        (@(Get-ChildItem -LiteralPath $Target -Recurse -File | Sort-Object FullName | ForEach-Object {
            "$($_.FullName.Substring($Target.Length))=$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash)"
        })) -join "`n"
    }
    # Inno's bootstrap launches a temporary setup child. Track only children of
    # our known live processes, retaining start times to exclude recycled IDs.
    $Started = [Collections.Generic.List[Diagnostics.Process]]::new()
    $Identities = @{}
    function Add-KnownProcess([Diagnostics.Process]$Process) {
        try {
            $Stamp = $Process.StartTime.ToUniversalTime().Ticks
            if (!$Identities.ContainsKey($Process.Id)) {
                $Identities[$Process.Id] = $Stamp
                $Started.Add($Process)
            } else { $Process.Dispose() }
        } catch { $Process.Dispose() }
    }
    function Update-KnownChildren {
        $LiveIds = @{}
        foreach ($Process in $Started) {
            $Process.Refresh()
            if (!$Process.HasExited -and $Process.StartTime.ToUniversalTime().Ticks -eq $Identities[$Process.Id]) {
                $LiveIds[$Process.Id] = $true
            }
        }
        foreach ($Row in @(Get-CimInstance Win32_Process)) {
            if ($LiveIds.ContainsKey([int]$Row.ParentProcessId) -and !$Identities.ContainsKey([int]$Row.ProcessId)) {
                try { Add-KnownProcess (Get-Process -Id $Row.ProcessId -ErrorAction Stop) }
                catch [Microsoft.PowerShell.Commands.ProcessCommandException] { }
            }
        }
    }
    function Start-Known([string]$Path, [string[]]$Arguments) {
        $Process = Start-Process -FilePath $Path -ArgumentList $Arguments -PassThru
        Add-KnownProcess $Process
        return $Process
    }
    function Wait-KnownExit([Diagnostics.Process]$Process, [string]$Context) {
        $Deadline = [DateTime]::UtcNow.AddSeconds(60)
        do {
            Update-KnownChildren
            if ($Process.WaitForExit(100)) { return $Process.ExitCode }
        } while ([DateTime]::UtcNow -lt $Deadline)
        throw "$Context timed out"
    }
    function Close-KnownProcesses {
        $Deadline = [DateTime]::UtcNow.AddSeconds(30)
        do {
            Update-KnownChildren
            $Alive = $false
            foreach ($Process in $Started) {
                $Process.Refresh()
                if ($Process.HasExited) { continue }
                $Alive = $true
                # Every window comes from the recorded process ID, never a
                # title match that could close an unrelated installer.
                $null = $Process.CloseMainWindow()
                foreach ($Window in [NativeSetupContentionWindows]::Windows($Process.Id)) {
                    $Yes = [NativeSetupContentionWindows]::GetDlgItem($Window, 6)
                    if ($Yes -ne [IntPtr]::Zero) {
                        $null = [NativeSetupContentionWindows]::PostMessage($Yes, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero)
                    } else {
                        $null = [NativeSetupContentionWindows]::PostMessage($Window, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
                    }
                }
            }
            if (!$Alive) { return }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $Deadline)
        throw 'Known contention fixture processes did not close gracefully within 30 seconds; no process was killed'
    }
    $Target = (Resolve-Path -LiteralPath $Target).Path.TrimEnd('\')
    $Exe = Join-Path $Target 'AXE-Voice.exe'
    $Uninstaller = Join-Path $Target 'unins000.exe'
    if (!(Test-Path -LiteralPath $Exe) -or !(Test-Path -LiteralPath $Uninstaller)) {
        throw 'Contention fixture requires an installed candidate and its uninstaller'
    }
    if ((Test-Marker $SetupMutexName) -or (Test-Marker $ApplicationMutexName)) {
        throw 'Candidate setup/application marker already exists before contention fixture'
    }
    $Before = Get-PayloadSnapshot
    try {
        $Wizard = Start-Known $Installer @('/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$Target`"")
        $Deadline = [DateTime]::UtcNow.AddSeconds(30)
        $Ready = $false
        do {
            Update-KnownChildren
            $HasWindow = $false
            foreach ($Process in $Started) {
                $Process.Refresh()
                if (!$Process.HasExited -and [NativeSetupContentionWindows]::Windows($Process.Id).Length -gt 0) {
                    $HasWindow = $true
                }
            }
            $Ready = $HasWindow -and (Test-Marker $SetupMutexName)
            if (!$Ready) { Start-Sleep -Milliseconds 100 }
        } while (!$Ready -and [DateTime]::UtcNow -lt $Deadline)
        if (!$Ready) { throw 'Interactive production installer did not expose a window and setup marker' }
        $Blocked = Start-Known $Exe @('--native-ui-smoke')
        $Code = Wait-KnownExit $Blocked 'Application launch during setup'
        if ($Code -ne $BusyExitCode) { throw "Application returned $Code during setup, expected $BusyExitCode" }
        $Second = Start-Known $Installer @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$Target`"")
        if ((Wait-KnownExit $Second 'Second installer') -eq 0) { throw 'Second installer accepted concurrent setup' }
        $Removal = Start-Known $Uninstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
        if ((Wait-KnownExit $Removal 'Concurrent uninstall') -eq 0) { throw 'Uninstaller accepted concurrent setup' }
        if (!(Test-Marker $SetupMutexName)) { throw 'First installer lost its setup reservation during contention' }
        if ((Get-PayloadSnapshot) -cne $Before) { throw 'Contending setup/uninstall changed installed payload' }
        Close-KnownProcesses
        $Deadline = [DateTime]::UtcNow.AddSeconds(10)
        while ((Test-Marker $SetupMutexName) -and [DateTime]::UtcNow -lt $Deadline) { Start-Sleep -Milliseconds 100 }
        if (Test-Marker $SetupMutexName) { throw 'Setup marker survived graceful installer cancellation' }
        if ((Get-PayloadSnapshot) -cne $Before) { throw 'Cancelling the setup wizard changed installed payload' }
        $Released = Start-Known $Exe @('--native-ui-smoke')
        if ((Wait-KnownExit $Released 'Application launch after cancellation') -ne 0) {
            throw 'Application smoke failed after the real installer released its reservation'
        }
    } finally {
        try { Close-KnownProcesses }
        finally { foreach ($Process in $Started) { $Process.Dispose() } }
    }
    Write-Output 'Production installer contention, graceful cancellation, payload preservation, and subsequent application launch passed'
}
