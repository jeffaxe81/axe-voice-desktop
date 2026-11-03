# Standalone Windows regression for the candidate uninstall readiness probe.
# No actual install or removal: test real named mutexes and kernel file sharing.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'native_uninstall_readiness.ps1')
$Suffix = [guid]::NewGuid().ToString('N')
$AppMarker = "Local\AXVUninstallTestApp.$Suffix"
$SetupMarker = "Local\AXVUninstallTestSetup.$Suffix"
$Target = Join-Path ([IO.Path]::GetTempPath()) "AXV-uninstall-preflight-$Suffix"
$Exe = Join-Path $Target 'AXE-Voice.exe'
New-Item -ItemType Directory -Force $Target | Out-Null
[IO.File]::WriteAllText($Exe,'fake candidate for readiness only')
function Assert-Blocked([string]$ExpectedReason) {
    $Result = Get-NativeUninstallReadiness -Executable $Exe -ApplicationMutexName $AppMarker -SetupMutexName $SetupMarker
    if ($Result.Ready -or $Result.Reason -notlike "*$ExpectedReason*") {
        throw "Uninstall preflight incorrectly allowed unsafe state: $($Result.Reason)"
    }
    $Failed = $false
    try {
        [void](Wait-NativeUninstallReadiness -Executable $Exe -ApplicationMutexName $AppMarker -SetupMutexName $SetupMarker -TimeoutSeconds 0)
    } catch {
        if ($_.Exception.Message -notlike '*failed closed*') { throw }
        $Failed = $true
    }
    if (!$Failed) { throw "Bounded uninstall preflight allowed $ExpectedReason" }
    if (!(Test-Path -LiteralPath $Exe -PathType Leaf)) { throw 'Blocked preflight modified the payload' }
}
try {
    $Ready = Wait-NativeUninstallReadiness -Executable $Exe -ApplicationMutexName $AppMarker -SetupMutexName $SetupMarker -TimeoutSeconds 0
    if (!$Ready.Ready) { throw 'Unblocked fixture was not ready' }

    $Created = $false
    $App = [Threading.Mutex]::new($false,$AppMarker,[ref]$Created)
    try {
        if (!$Created) { throw 'App marker fixture collision' }
        Assert-Blocked 'application marker'
    } finally { $App.Dispose() }

    $Created = $false
    $Setup = [Threading.Mutex]::new($false,$SetupMarker,[ref]$Created)
    try {
        if (!$Created) { throw 'Setup marker fixture collision' }
        Assert-Blocked 'setup marker'
    } finally { $Setup.Dispose() }

    $Open = [IO.File]::Open($Exe,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
    try { Assert-Blocked 'EXE not exclusively accessible' }
    finally { $Open.Dispose() }

    # Missing payload must NOT be treated as safe, even when both mutexes are absent.
    Remove-Item -LiteralPath $Exe -Force
    $Missing = Get-NativeUninstallReadiness -Executable $Exe -ApplicationMutexName $AppMarker -SetupMutexName $SetupMarker
    if ($Missing.Ready) { throw 'Missing executable incorrectly marked ready' }
    [IO.File]::WriteAllText($Exe,'restored candidate fixture')
    $Ready = Wait-NativeUninstallReadiness -Executable $Exe -ApplicationMutexName $AppMarker -SetupMutexName $SetupMarker -TimeoutSeconds 0
    if (!$Ready.Ready) { throw 'Ready state did not recover after releasing guards' }
} finally {
    Remove-Item -LiteralPath $Target -Force -Recurse -ErrorAction SilentlyContinue
}
Write-Output 'Fail-closed uninstall preflight passed: app/setup markers, locked/missing EXE, bounded wait, recovery'
