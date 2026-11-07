# Acceptance check for existing Windows installer requirements.
# The desktop shortcut must be optional, scoped to the current user, branded,
# and removed by Inno Setup when the candidate is uninstalled.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$spec=Get-Content -LiteralPath (Join-Path $root 'installer/axe-native-candidate.iss') -Raw -Encoding UTF8
$group = [regex]::Match($spec,'(?ms)^\[Tasks\]\s*\r?\n(?<body>.*?)(?=^\[)')
$icons = [regex]::Match($spec,'(?ms)^\[Icons\]\s*\r?\n(?<body>.*?)(?=^\[)')
if (!$group.Success -or !$icons.Success) { throw 'Missing installer Tasks or Icons section' }
$task=@($group.Groups['body'].Value -split "\r?\n" | Where-Object {
    $_ -match '^Name:\s*"desktopicon";'
})
if($task.Count -ne 1 -or $task[0] -notmatch 'Flags:\s*unchecked(?:\s|$)') {
    throw 'Desktop shortcut task must be declared once and unchecked by default'
}
$links=@($icons.Groups['body'].Value -split "\r?\n" | Where-Object {
    $_ -match '^Name:\s*"\{autodesktop\}\\AXE Voice Native Candidate";'
})
if($links.Count -ne 1 -or $links[0] -notmatch 'Filename:\s*"\{app\}\\AXE-Voice\.exe";\s*Tasks:\s*desktopicon') {
    throw 'Desktop shortcut must point to the branded installed EXE and require explicit opt-in'
}
if ($spec -notmatch '(?m)^PrivilegesRequired=lowest\s*$' -or
    $spec -notmatch '(?m)^UninstallDisplayIcon=\{app\}\\AXE-Voice\.exe\s*$') {
    throw 'Per-user installer or uninstall icon contract changed'
}
Write-Host 'Native candidate optional Windows desktop shortcut contract: PASS'
