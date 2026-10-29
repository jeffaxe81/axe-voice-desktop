# Windows behavioral acceptance checks, dot-sourced by candidate packaging.
# The first case deliberately holds no executable or DLL open: only the named
# application mutex can prevent this upgrade, so Restart Manager cannot mask it.
function Invoke-NativeUpgradeGuardChecks {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Installer,
        [Parameter(Mandatory)][string]$Target,
        [Parameter(Mandatory)][string]$Stage,
        [Parameter(Mandatory)][string]$ApplicationMutexName,
        [Parameter(Mandatory)][string]$SetupMutexName,
        [Parameter(Mandatory)][int]$BusyExitCode
    )
    $ErrorActionPreference = 'Stop'
    $Payload = @(Get-ChildItem -LiteralPath $Stage -Recurse -File | ForEach-Object {
        Join-Path $Target $_.FullName.Substring($Stage.Length + 1)
    })
    function Get-PayloadSnapshot {
        ($Payload | Sort-Object | ForEach-Object {
            "$_=$((Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash)"
        }) -join "`n"
    }
    function Invoke-Upgrade {
        $Process = Start-Process -FilePath $Installer -ArgumentList @(
            '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$Target`"") -PassThru
        try {
            if (!$Process.WaitForExit(60000)) { throw 'Candidate upgrade timed out' }
            $Process.ExitCode
        } finally { $Process.Dispose() }
    }
    function Test-ApplicationMarker {
        try { $Marker = [Threading.Mutex]::OpenExisting($ApplicationMutexName) }
        catch [Threading.WaitHandleCannotBeOpenedException] { return $false }
        # Do not retain a probe handle: that would keep the marker alive itself
        # and hide the GUI incorrectly closing its own handle during the test.
        try { return $true } finally { $Marker.Dispose() }
    }
    function Assert-RejectedUpgrade([string]$Context) {
        $Before = Get-PayloadSnapshot
        $Code = Invoke-Upgrade
        if ($Code -eq 0) { throw "$Context`: installer accepted an upgrade while the application guard was held" }
        if ((Get-PayloadSnapshot) -cne $Before) { throw "$Context`: rejected upgrade changed installed payload" }
    }
    function Assert-RejectedUninstall([string]$Context) {
        $Before = Get-PayloadSnapshot
        $Process = Start-Process -FilePath (Join-Path $Target 'unins000.exe') -ArgumentList @(
            '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -PassThru
        try {
            if (!$Process.WaitForExit(60000)) { throw "$Context`: candidate uninstall timed out" }
            if ($Process.ExitCode -eq 0) { throw "$Context`: uninstaller accepted removal while the application guard was held" }
            if ((Get-PayloadSnapshot) -cne $Before) { throw "$Context`: rejected uninstall changed installed payload" }
        } finally { $Process.Dispose() }
    }
    function Assert-UpgradeSucceeds {
        if ((Invoke-Upgrade) -ne 0) { throw 'Candidate upgrade failed after the application guard was released' }
    }
    # Open with exclusive sharing and immediately close every payload file. The
    # regression must fail on an unguarded installer even with unlocked files.
    foreach ($Path in $Payload) {
        $Stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
        $Stream.Dispose()
    }
    $Created = $false
    $Guard = [Threading.Mutex]::new($false, $ApplicationMutexName, [ref]$Created)
    try {
        if (!$Created) { throw 'Application guard already held before packaging regression' }
        Assert-RejectedUpgrade 'Unlocked payload regression'
        Assert-RejectedUninstall 'Unlocked payload regression'
    } finally {
        $Guard.Dispose()
    }
    Assert-UpgradeSucceeds

    # Exercise the installed GUI itself, including its lifetime ownership and
    # continued operation after setup rejects the upgrade.
    $App = Start-Process -FilePath (Join-Path $Target 'AXE-Voice.exe') -PassThru
    try {
        $Deadline = [DateTime]::UtcNow.AddSeconds(20)
        $Ready = $false
        do {
            $App.Refresh()
            if ($App.HasExited) { throw 'Installed candidate exited before acquiring its application guard' }
            $Ready = (Test-ApplicationMarker) -and $App.MainWindowHandle -ne [IntPtr]::Zero
            if (!$Ready) { Start-Sleep -Milliseconds 100 }
        } while (!$Ready -and [DateTime]::UtcNow -lt $Deadline)
        if (!$Ready) { throw 'Installed GUI did not expose a window and hold its application guard' }
        Assert-RejectedUpgrade 'Running installed GUI regression'
        Assert-RejectedUninstall 'Running installed GUI regression'
        $App.Refresh()
        if ($App.HasExited) { throw 'Rejected upgrade terminated the installed GUI' }
        if (!(Test-ApplicationMarker)) { throw 'Installed GUI released its guard while still running' }
    } finally {
        try {
            $App.Refresh()
            if (!$App.HasExited) {
                if (!$App.CloseMainWindow()) { throw 'Could not request graceful candidate GUI shutdown' }
                if (!$App.WaitForExit(30000)) { throw 'Candidate GUI did not close gracefully' }
            }
        } finally { $App.Dispose() }
    }
    Assert-UpgradeSucceeds

    $Created = $false
    $SetupGuard = [Threading.Mutex]::new($false, $SetupMutexName, [ref]$Created)
    try {
        if (!$Created) { throw 'Setup guard already held before reciprocal launch regression' }
        $Blocked = Start-Process -FilePath (Join-Path $Target 'AXE-Voice.exe') -ArgumentList '--native-ui-smoke' -PassThru
        try {
            if (!$Blocked.WaitForExit(15000)) { throw 'GUI did not promptly refuse launch while setup held its guard' }
            if ($Blocked.ExitCode -ne $BusyExitCode) {
                throw "GUI setup-busy exit code was $($Blocked.ExitCode), expected $BusyExitCode"
            }
        } finally { $Blocked.Dispose() }
    } finally {
        $SetupGuard.Dispose()
    }
    Write-Output 'Candidate guard rejects unlocked and live-GUI upgrades/uninstalls without terminating the app; reciprocal launch guard passed'
}
