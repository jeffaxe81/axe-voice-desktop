$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'native_setup_cancel_policy.ps1')
$Expected = "Setup is not complete. If you exit now, the program will not be installed.`r`n`r`nYou may run Setup again at another time to complete the installation.`r`n`r`nExit Setup?"
function Assert-Decision($ExpectedDecision, $Class, $Title, $Text, $Buttons) {
    $Actual = Get-NativeSetupCancelDecision -WindowClass $Class -Title $Title -Text $Text -ButtonIds $Buttons
    if ($Actual -cne $ExpectedDecision) { throw "Expected $ExpectedDecision, got $Actual" }
}
Assert-Decision 'ConfirmCancel' '#32770' 'Exit Setup' $Expected @(6,7)
Assert-Decision 'UnexpectedDialog' '#32770' 'Delete files?' 'Delete all files?' @(6,7)
Assert-Decision 'UnexpectedDialog' '#32770' 'Other title' $Expected @(6,7)
Assert-Decision 'UnexpectedDialog' '#32770' 'Exit Setup' 'Unexpected contents' @(6,7)
Assert-Decision 'UnexpectedDialog' 'TNewForm' 'Exit Setup' $Expected @(6,7)
Assert-Decision 'UnexpectedDialog' '#32770' 'Exit Setup' $Expected @(6)
Assert-Decision 'UnexpectedDialog' '#32770' 'Exit Setup' $Expected @(6,7,2)
Assert-Decision 'CloseWindow' 'TWizardForm' 'AXE Setup' '' @(2)
Write-Output 'Native setup cancellation policy passed'
