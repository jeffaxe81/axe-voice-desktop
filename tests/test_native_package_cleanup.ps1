$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'native_package_cleanup.ps1')
foreach ($AcceptanceFails in @($false, $true)) {
    foreach ($CleanupFails in @($false, $true)) {
        $State = @{ Acceptance = 0; Cleanup = 0 }
        $Primary = [InvalidOperationException]::new('postinstall acceptance failed')
        $Secondary = [InvalidOperationException]::new('silent uninstall failed')
        $Caught = $null
        $Warnings = @()
        $Output = @()
        try {
            $Output = @(Invoke-NativePackageAcceptance -Acceptance {
                $State.Acceptance++
                if ($AcceptanceFails) { throw $Primary }
                'acceptance output'
            } -Cleanup {
                $State.Cleanup++
                if ($CleanupFails) { throw $Secondary }
            } -WarningVariable Warnings -WarningAction SilentlyContinue)
        } catch { $Caught = $_ }
        if ($State.Acceptance -ne 1 -or $State.Cleanup -ne 1) { throw 'Each phase must execute exactly once' }
        if ($AcceptanceFails) {
            if (!$Caught -or ![object]::ReferenceEquals($Caught.Exception, $Primary)) {
                throw "Primary acceptance error was replaced (cleanup failure: $CleanupFails): $Caught"
            }
        } elseif ($CleanupFails) {
            if (!$Caught -or ![object]::ReferenceEquals($Caught.Exception, $Secondary)) { throw 'Cleanup-only failure must fail acceptance' }
        } else {
            if ($Caught -or $Output.Count -ne 1 -or $Output[0] -ne 'acceptance output') { throw 'Successful acceptance must preserve output' }
        }
        if ($AcceptanceFails -and $CleanupFails) {
            if ($Warnings.Count -ne 1 -or $Warnings[0].Message -notlike '*silent uninstall failed*') { throw 'Secondary failure must be reported as a cleanup warning' }
        } elseif ($Warnings.Count -ne 0) { throw 'Unexpected cleanup warning' }
    }
}
Write-Output 'Native package cleanup: all four acceptance/cleanup outcomes passed'
