[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildDir=Join-Path $root 'build'
if (-not (Test-Path -LiteralPath $buildDir -PathType Container)) {
    throw 'Build nao existe: execute scripts/build-windows.ps1 antes.'
}
$groups=@('unit','blackbox','integration')
$raw=& ctest --test-dir $buildDir -C $Configuration --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'Nao foi possivel ler o manifesto CTest.' }
$manifest=($raw -join [Environment]::NewLine) | ConvertFrom-Json
$summary=[ordered]@{}
foreach($group in $groups) {
    Write-Host "=== AXE VOICE QUALITY GATE: $group ==="
    $matching=@($manifest.tests | Where-Object {
        $test=$_
        @($test.properties | Where-Object {
            $_.name -eq 'LABELS' -and @($_.value) -contains $group
        }).Count -gt 0
    })
    if ($matching.Count -eq 0) { throw "Gate '$group' sem testes." }
    & ctest --test-dir $buildDir -C $Configuration --output-on-failure -L "^$group$"
    if ($LASTEXITCODE -ne 0) { throw "Gate '$group' FALHOU." }
    if ($group -eq 'integration') {
        # Exercise the real owner-thread lifecycle repeatedly.
        Write-Host '=== AXE VOICE REACTOR STABILITY GATE: 10 repetitions ==='
        & ctest --test-dir $buildDir -C $Configuration --output-on-failure -R '^libre_owner_reactor$' --repeat until-fail:10
        if ($LASTEXITCODE -ne 0) { throw 'Gate libre_owner_reactor stability FALHOU.' }
        Write-Host 'Reactor stability gate APROVADO: 10/10'
        # A separate CTest process executes 1400 deterministic CTI scenarios.
        # Repeat three times and fail if any scenario fails.
        Write-Host '=== AXE VOICE CTI MATRIX: 3 x 1400 scenarios ==='
        & ctest --test-dir $buildDir -C $Configuration --output-on-failure -R '^local_cti_stress_matrix' --repeat until-fail:3
        if ($LASTEXITCODE -ne 0) { throw 'CTI 1400-scenario matrix FALHOU.' }
        Write-Host 'CTI matrix gate APROVADO: 3 x 1400 scenarios'
    }
    $summary[$group]=$matching.Count
    Write-Host "GATE $group APROVADO: $($matching.Count)/$($matching.Count)"
}
# Eight full-suite cycles independently exercise all AXE Voice CTest modules.
# At least 1000 actual CTest executions must be scheduled; fail on any error.
$fullCycles=8
$fullExecutions=@($manifest.tests).Count * $fullCycles
if ($fullExecutions -lt 1000) {
    throw "Full suite stress below 1000 requested executions: $fullExecutions"
}
Write-Host "=== AXE VOICE FULL TEST SUITE: $(@($manifest.tests).Count) tests x $fullCycles ==="
& ctest --test-dir $buildDir -C $Configuration --output-on-failure --repeat until-fail:8
if ($LASTEXITCODE -ne 0) { throw 'Full-suite stress FALHOU.' }
Write-Host "Full-suite stress APROVADO: $fullExecutions/$fullExecutions CTest executions"
$total=0
foreach($n in $summary.Values) { $total+=$n }
Write-Host "AXE Voice - 100% de PASSAGEM dos testes categorizados: $total/$total"
Write-Host 'Isto NAO significa 100% de cobertura de codigo ou homologacao SIP real.'
