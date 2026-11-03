# Fail-closed readiness probe for the candidate's silent uninstall.
# Checks protected application/setup markers and exclusive access to its EXE.
# Never terminates processes or overrides the production Inno guard.
function Get-NativeUninstallReadiness {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string]$ApplicationMutexName,
        [Parameter(Mandatory)][string]$SetupMutexName
    )
    foreach ($Check in @(
        @{ Name=$ApplicationMutexName; Label='application marker' },
        @{ Name=$SetupMutexName; Label='setup marker' }
    )) {
        $Marker = $null
        try {
            $Marker = [Threading.Mutex]::OpenExisting($Check.Name)
            return [pscustomobject]@{ Ready=$false; Reason="$($Check.Label) still present" }
        } catch [Threading.WaitHandleCannotBeOpenedException] {
            # Absent marker. Still require the other marker and EXE checks.
        } catch {
            # Unexpected error is NOT evidence that the marker is absent.
            return [pscustomobject]@{ Ready=$false; Reason="$($Check.Label) could not be verified: $($_.Exception.GetType().Name)" }
        } finally {
            if ($null -ne $Marker) { $Marker.Dispose() }
        }
    }
    $Stream = $null
    try {
        $Stream = [IO.File]::Open($Executable,[IO.FileMode]::Open,
                                 [IO.FileAccess]::Read,[IO.FileShare]::None)
        return [pscustomobject]@{ Ready=$true; Reason='No markers and EXE exclusively accessible' }
    } catch {
        return [pscustomobject]@{ Ready=$false; Reason="Installed EXE not exclusively accessible: $($_.Exception.GetType().Name)" }
    } finally {
        if ($null -ne $Stream) { $Stream.Dispose() }
    }
}

function Wait-NativeUninstallReadiness {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string]$ApplicationMutexName,
        [Parameter(Mandatory)][string]$SetupMutexName,
        [ValidateRange(0,60)][int]$TimeoutSeconds=15
    )
    $Clock=[Diagnostics.Stopwatch]::StartNew()
    do {
        $Observation=Get-NativeUninstallReadiness -Executable $Executable -ApplicationMutexName $ApplicationMutexName -SetupMutexName $SetupMutexName
        if ($Observation.Ready) { return $Observation }
        if ($Clock.Elapsed.TotalSeconds -ge $TimeoutSeconds) { break }
        Start-Sleep -Milliseconds 150
    } while ($true)
    throw "Candidate uninstall readiness check failed closed: $($Observation.Reason)"
}
