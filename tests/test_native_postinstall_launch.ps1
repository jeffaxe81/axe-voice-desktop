# Real production Inno [Run] acceptance check. Uses Win32 window messages on
# the runner desktop at the same integrity level as this PowerShell process.
# /SILENT and /VERYSILENT both suppress entries marked skipifsilent, so neither
# is used. No direct application launch or test-only installer is substituted.
function Invoke-NativePostinstallLaunchCheck {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Installer,
        [Parameter(Mandatory)][string]$Target,
        [Parameter(Mandatory)][string]$ApplicationMutexName,
        [Parameter(Mandatory)][string]$SetupMutexName
    )
    $ErrorActionPreference = 'Stop'
    . (Join-Path $PSScriptRoot 'native_postinstall_identity.ps1')
    if ($null -eq ('NativePostinstallWindows' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class NativePostinstallWindows {
    public delegate bool Callback(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback, IntPtr parameter);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr parent, Callback callback, IntPtr parameter);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll", SetLastError=true)] static extern IntPtr SendMessageTimeout(IntPtr window, uint message, IntPtr wparam, IntPtr lparam, uint flags, uint timeout, out IntPtr result);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    public static IntPtr[] Windows(IntPtr parent) {
        var list = new List<IntPtr>();
        Callback callback = (window, parameter) => { list.Add(window); return true; };
        if (parent == IntPtr.Zero) EnumWindows(callback, IntPtr.Zero);
        else EnumChildWindows(parent, callback, IntPtr.Zero);
        return list.ToArray();
    }
    public static string Class(IntPtr window) { var text = new StringBuilder(256); GetClassName(window, text, text.Capacity); return text.ToString(); }
    public static string Text(IntPtr window) { var text = new StringBuilder(1024); GetWindowText(window, text, text.Capacity); return text.ToString(); }
    public static void Click(IntPtr window) {
        // Installation can block the button handler while copying files. Post
        // the click and observe the next page under the outer bounded deadline.
        if (!PostMessage(window, 0x00F5, IntPtr.Zero, IntPtr.Zero))
            throw new InvalidOperationException("Could not post wizard BM_CLICK");
    }
}
'@
    }
    function Test-Marker([string]$Name) {
        try { $Handle = [Threading.Mutex]::OpenExisting($Name) }
        catch [Threading.WaitHandleCannotBeOpenedException] { return $false }
        try { return $true } finally { $Handle.Dispose() }
    }
    $Installer = (Resolve-Path -LiteralPath $Installer).Path
    $Exe = [IO.Path]::GetFullPath((Join-Path $Target 'AXE-Voice.exe'))
    $Known = @{}
    function Get-InstalledApp {
        @($Known.Values | Where-Object { $_.IsApp } | ForEach-Object { $_.Process })
    }
    # Path discovery is only a precondition, never attribution or cleanup.
    $Existing = @(Get-Process -Name 'AXE-Voice' -ErrorAction SilentlyContinue | Where-Object {
        $_.Path -and [string]::Equals($_.Path, $Exe, [StringComparison]::OrdinalIgnoreCase)
    })
    try {
        if ((Test-Marker $ApplicationMutexName) -or (Test-Marker $SetupMutexName) -or $Existing.Count) {
            throw 'Postinstall check requires no running candidate GUI or setup'
        }
    } finally { foreach ($Process in $Existing) { $Process.Dispose() } }
    function Update-OwnedProcesses {
        foreach ($Identity in $Known.Values) {
            $Identity.Process.Refresh()
            if ($Identity.Process.HasExited -and $null -eq $Identity.ExitedAt) {
                $Identity.ExitedAt = $Identity.Process.ExitTime.ToUniversalTime()
            }
        }
        $Rows = @(Get-CimInstance Win32_Process)
        do {
            $Added = $false
            foreach ($Row in $Rows) {
                $Parent = $Known[[uint32]$Row.ParentProcessId]
                if (!$Parent -or $Parent.IsApp -or $Known.ContainsKey([uint32]$Row.ProcessId)) { continue }
                $Parent.Process.Refresh()
                if ($Parent.Process.HasExited) { $Parent.ExitedAt = $Parent.Process.ExitTime.ToUniversalTime() }
                if (!(Test-NativePostinstallChildIdentity ([uint32]$Row.ProcessId) ([uint32]$Row.ParentProcessId) $Row.CreationDate $Parent)) { continue }
                try { $Process = Get-Process -Id $Row.ProcessId -ErrorAction Stop }
                catch [Microsoft.PowerShell.Commands.ProcessCommandException] { continue }
                try {
                    [void]$Process.Handle # retain the kernel identity across PID reuse
                    if (!(Test-NativePostinstallSameIdentity $Row.CreationDate $Process.StartTime)) { continue }
                    $IsApp = $Row.Name -eq 'AXE-Voice.exe' -and $Row.ExecutablePath -and
                        [string]::Equals($Row.ExecutablePath, $Exe, [StringComparison]::OrdinalIgnoreCase)
                    $Known[[uint32]$Process.Id] = [pscustomobject]@{
                        Process=$Process; Id=[uint32]$Process.Id; StartedAt=$Process.StartTime.ToUniversalTime(); ExitedAt=$null; IsApp=[bool]$IsApp
                    }
                    $Process = $null # ownership transferred to Known
                    $Added = $true
                } finally { if ($Process) { $Process.Dispose() } }
            }
        } while ($Added)
    }
    function Test-OwnedSetupWindow([IntPtr]$Window) {
        $OwnerId = [uint32]0
        [void][NativePostinstallWindows]::GetWindowThreadProcessId($Window, [ref]$OwnerId)
        $Identity = $Known[$OwnerId]
        if (!$Identity -or $Identity.IsApp) { return $false }
        $Identity.Process.Refresh()
        return !$Identity.Process.HasExited -and (Test-NativePostinstallSameIdentity $Identity.StartedAt $Identity.Process.StartTime)
    }
    $Setup = $null
    $App = $null
    $Wizard = [IntPtr]::Zero
    $SawFinish = $false
    $SawWizard = $false
    # Track the bootstrapper and its setup engine only. AXE is also a descendant
    # but must remain alive when setup exits, so exclude it from setup liveness.
    try {
        $Setup = Start-Process -FilePath $Installer -ArgumentList @(
            '/SP-', '/SUPPRESSMSGBOXES', '/NORESTART', "/DIR=`"$Target`"") -PassThru
        [void]$Setup.Handle # keep bootstrap identity valid after its exit
        $Known[[uint32]$Setup.Id] = [pscustomobject]@{
            Process=$Setup; Id=[uint32]$Setup.Id; StartedAt=$Setup.StartTime.ToUniversalTime(); ExitedAt=$null; IsApp=$false
        }
        $Deadline = [DateTime]::UtcNow.AddSeconds(120)
        $LastAction = ''
        do {
            Update-OwnedProcesses
            foreach ($Window in [NativePostinstallWindows]::Windows([IntPtr]::Zero)) {
                if (!(Test-OwnedSetupWindow $Window) -or ![NativePostinstallWindows]::IsWindowVisible($Window)) { continue }
                if ([NativePostinstallWindows]::Class($Window) -ne 'TWizardForm') { continue }
                $Wizard = $Window
                $SawWizard = $true
                $Controls = @([NativePostinstallWindows]::Windows($Wizard))
                $Page = ($Controls | Where-Object { [NativePostinstallWindows]::IsWindowVisible($_) } |
                    ForEach-Object { [NativePostinstallWindows]::Text($_) }) -join "|"
                foreach ($Control in $Controls) {
                    if (![NativePostinstallWindows]::IsWindowVisible($Control) -or
                        ![NativePostinstallWindows]::IsWindowEnabled($Control) -or
                        [NativePostinstallWindows]::Class($Control) -notin @('TNewButton', 'TButton')) { continue }
                    $Caption = [NativePostinstallWindows]::Text($Control).Replace('&','').Trim()
                    # The checked-by-default postinstall checklist is untouched.
                    # Only the stock English wizard navigation is automated;
                    # any unexpected/localized dialog fails within the deadline.
                    if ($Caption -notin @('Next >', 'Install', 'Finish')) { continue }
                    $Action = "$Control/$Caption/$Page"
                    if ($Action -eq $LastAction) { continue }
                    if ($Caption -eq 'Finish') { $SawFinish = $true }
                    [NativePostinstallWindows]::Click($Control)
                    $LastAction = $Action
                    break
                }
            }
            $Setup.Refresh()
            $ActiveSetup = @($Known.Values | Where-Object { !$_.IsApp -and !$_.Process.HasExited })
            if ($Setup.HasExited -and $ActiveSetup.Count -eq 0) { break }
            Start-Sleep -Milliseconds 200
        } while ([DateTime]::UtcNow -lt $Deadline)
        if (!$Setup.HasExited -or $ActiveSetup.Count) { throw 'Interactive setup timed out; inspect the desktop for an unexpected wizard/dialog' }
        if ($Setup.ExitCode -ne 0) { throw "Interactive setup failed with exit code $($Setup.ExitCode)" }
        if (!$SawWizard -or !$SawFinish) { throw 'Did not observe and finish the real interactive Inno wizard' }
        if (Test-Marker $SetupMutexName) { throw 'Setup marker remained after interactive setup exited' }
        $Deadline = [DateTime]::UtcNow.AddSeconds(30)
        do {
            Update-OwnedProcesses
            $Apps = @(Get-InstalledApp)
            if ($Apps.Count -gt 1) { throw 'Postinstall launched multiple candidate GUIs' }
            if ($Apps.Count -eq 1) {
                $App = $Apps[0]
                $App.Refresh()
                if (!$App.HasExited -and $App.MainWindowHandle -ne [IntPtr]::Zero -and
                    (Test-Marker $ApplicationMutexName)) { break }
            }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $Deadline)
        if (!$App -or $App.HasExited -or $App.MainWindowHandle -eq [IntPtr]::Zero -or
            !(Test-Marker $ApplicationMutexName)) { throw 'Production postinstall [Run] did not leave a real GUI holding the application marker' }
        if (Test-Marker $SetupMutexName) { throw 'Setup marker exists while postinstall GUI is running' }
    } finally {
        # Never terminate AXE or setup: request ordinary window shutdown even on
        # failure. Unexpected modal dialogs may require operator cleanup.
        if ($Setup) {
            $Setup.Refresh()
            if ($Wizard -ne [IntPtr]::Zero -and (Test-OwnedSetupWindow $Wizard)) {
                [void][NativePostinstallWindows]::PostMessage($Wizard, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
            }
        }
        foreach ($Running in @(Get-InstalledApp)) {
            try {
                $Running.Refresh()
                if (!$Running.HasExited) {
                    if (!$Running.CloseMainWindow()) { throw 'Could not request graceful postinstall GUI shutdown' }
                    if (!$Running.WaitForExit(30000)) { throw 'Postinstall GUI did not close gracefully' }
                }
            } finally { }
        }
        foreach ($Identity in $Known.Values) { $Identity.Process.Dispose() }
    }
    if (Test-Marker $ApplicationMutexName) { throw 'Application marker remained after graceful postinstall GUI shutdown' }
    Write-Output 'Production interactive Inno postinstall [Run] launched the installed GUI, with setup marker absent and application marker held; GUI closed gracefully'
}
