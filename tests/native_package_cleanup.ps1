# Run acceptance and always release its installed candidate.
function Invoke-NativePackageAcceptance {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][scriptblock]$Acceptance,
        [Parameter(Mandatory)][scriptblock]$Cleanup
    )
    $PrimaryError = $null
    try {
        & $Acceptance
    } catch {
        $PrimaryError = $_
        throw
    } finally {
        try { & $Cleanup } catch {
            if ($null -eq $PrimaryError) { throw }
            # Warning preferences must not turn this secondary diagnostic into
            # a terminating error that hides the acceptance failure again.
            Write-Warning "Candidate cleanup also failed: $($_.Exception.Message)" -WarningAction Continue
        }
    }
}
