# Inno Setup 6 English defaults, pinned upstream reference:
# https://github.com/jrsoftware/issrc/blob/is-6_4_3/Files/Default.isl
# ExitSetupTitle and ExitSetupMessage; %n expands to a Windows newline.
function Get-NativeSetupCancelDecision {
    param([string]$WindowClass, [string]$Title, [string]$Text, [int[]]$ButtonIds)
    $Expected = "Setup is not complete. If you exit now, the program will not be installed.`r`n`r`nYou may run Setup again at another time to complete the installation.`r`n`r`nExit Setup?"
    $Ids = @($ButtonIds | Sort-Object -Unique)
    # Inno's visible VCL dispatcher has no user content or actions. Never send
    # it a close/affirmative message; it is not a confirmation dialog.
    if ($WindowClass -ceq 'TApplication' -and $Title -ceq '' -and $Text -ceq '' -and $Ids.Count -eq 0) { return 'IgnoreWindow' }
    if ($WindowClass -ceq '#32770' -and $Title -ceq 'Exit Setup' -and $Text -ceq $Expected -and $Ids.Count -eq 2 -and $Ids[0] -eq 6 -and $Ids[1] -eq 7) { return 'ConfirmCancel' }
    if ($WindowClass -ceq '#32770' -or 6 -in $Ids -or 7 -in $Ids) { return 'UnexpectedDialog' }
    if ($WindowClass -ceq 'TWizardForm') { return 'CloseWindow' }
    return 'UnexpectedDialog'
}
