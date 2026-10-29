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
    function Get-InstalledApp {
        @(Get-Process -Name 'AXE-Voice' -ErrorAction SilentlyContinue | Where-Object {
            $_.Path -and [string]::Equals($_.Path, $Exe, [StringComparison]::OrdinalIgnoreCase)
        })
    }
    if ((Test-Marker $ApplicationMutexName) -or (Test-Marker $SetupMutexName) -or @(Get-InstalledApp).Count) {
        throw 'Postinstall check requires no running candidate GUI or setup'
    }
    $Setup = $null
    $App = $null
    $Wizard = [IntPtr]::Zero
    $SawFinish = $false
    $SawWizard = $false
    # Track the bootstrapper and its setup engine only. AXE is also a descendant
    # but must remain alive when setup exits, so exclude it from setup liveness.
    $SetupIds = [Collections.Generic.HashSet[uint32]]::new()
    try {
        $Setup = Start-Process -FilePath $Installer -ArgumentList @(
            '/SP-', '/SUPPRESSMSGBOXES', '/NORESTART', "/DIR=`"$Target`"") -PassThru
        [void]$SetupIds.Add([uint32]$Setup.Id)
        $Deadline = [DateTime]::UtcNow.AddSeconds(120)
        $LastAction = ''
        do {
            $Processes = @(Get-CimInstance Win32_Process)
            do {
                $Added = $false
                foreach ($Process in $Processes) {
                    if ($SetupIds.Contains([uint32]$Process.ParentProcessId) -and
                        $Process.Name -ne 'AXE-Voice.exe' -and !$SetupIds.Contains([uint32]$Process.ProcessId)) {
                        [void]$SetupIds.Add([uint32]$Process.ProcessId)
                        $Added = $true
                    }
                }
            } while ($Added)
            foreach ($Window in [NativePostinstallWindows]::Windows([IntPtr]::Zero)) {
                $OwnerId = [uint32]0
                [void][NativePostinstallWindows]::GetWindowThreadProcessId($Window, [ref]$OwnerId)
                if (!$SetupIds.Contains($OwnerId) -or ![NativePostinstallWindows]::IsWindowVisible($Window)) { continue }
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
            $ActiveSetup = @($Processes | Where-Object { $SetupIds.Contains([uint32]$_.ProcessId) })
            if ($Setup.HasExited -and $ActiveSetup.Count -eq 0) { break }
            Start-Sleep -Milliseconds 200
        } while ([DateTime]::UtcNow -lt $Deadline)
        if (!$Setup.HasExited -or $ActiveSetup.Count) { throw 'Interactive setup timed out; inspect the desktop for an unexpected wizard/dialog' }
        if ($Setup.ExitCode -ne 0) { throw "Interactive setup failed with exit code $($Setup.ExitCode)" }
        if (!$SawWizard -or !$SawFinish) { throw 'Did not observe and finish the real interactive Inno wizard' }
        if (Test-Marker $SetupMutexName) { throw 'Setup marker remained after interactive setup exited' }
        $Deadline = [DateTime]::UtcNow.AddSeconds(30)
        do {
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
            if (!$Setup.HasExited -and $Wizard -ne [IntPtr]::Zero) {
                [void][NativePostinstallWindows]::PostMessage($Wizard, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
            }
            $Setup.Dispose()
        }
        foreach ($Running in @(Get-InstalledApp)) {
            try {
                $Running.Refresh()
                if (!$Running.HasExited) {
                    if (!$Running.CloseMainWindow()) { throw 'Could not request graceful postinstall GUI shutdown' }
                    if (!$Running.WaitForExit(30000)) { throw 'Postinstall GUI did not close gracefully' }
                }
            } finally { $Running.Dispose() }
        }
        if ($App) { $App.Dispose() }
    }
    if (Test-Marker $ApplicationMutexName) { throw 'Application marker remained after graceful postinstall GUI shutdown' }
    Write-Output 'Production interactive Inno postinstall [Run] launched the installed GUI, with setup marker absent and application marker held; GUI closed gracefully'
}
