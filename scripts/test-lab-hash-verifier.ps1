# Offline end-to-end checks: never executes the softphone or touches real audio.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$SourceDir,
    [Parameter(Mandatory=$true)][string]$Verifier
)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([System.IO.Path]::GetTempPath()) ('AXV-hash-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root -Force | Out-Null
$exe = Join-Path $root 'AXE-Voice.exe'
$manifest = Join-Path $root 'AXE-Voice-LAB.sha256'
function Restore-Fixtures {
    Copy-Item -LiteralPath (Join-Path $SourceDir 'AXE-Voice.exe') -Destination $exe -Force
    Copy-Item -LiteralPath (Join-Path $SourceDir 'AXE-Voice-LAB.sha256') -Destination $manifest -Force
}
function Expect-Check([bool]$expected, [string]$scenario) {
    $passed = $true
    try {
        & $Verifier -Directory $root | Out-Null
    } catch {
        $passed = $false
    }
    if ($passed -ne $expected) {
        throw "Resultado inesperado do verificador: $scenario (sucesso=$passed)"
    }
}
try {
    Restore-Fixtures
    Expect-Check $true 'hash original'
    [System.IO.File]::AppendAllText($exe,'TAMPER',[System.Text.Encoding]::ASCII)
    Expect-Check $false 'binario adulterado'
    Restore-Fixtures
    [System.IO.File]::WriteAllText($manifest,('0' * 64 + '  AXE-Voice.exe' + "`n"))
    Expect-Check $false 'digest divergente'
    Restore-Fixtures
    [System.IO.File]::AppendAllText($manifest,('0' * 64 + '  AXE-Voice.exe' + "`n"))
    Expect-Check $false 'duas linhas no manifesto'
    Restore-Fixtures
    [System.IO.File]::WriteAllText($manifest,('0' * 64 + '  FORGED.exe' + "`n"))
    Expect-Check $false 'nome inesperado'
    Restore-Fixtures
    Remove-Item -LiteralPath $manifest -Force
    Expect-Check $false 'manifesto ausente'
    Restore-Fixtures
    Remove-Item -LiteralPath $exe -Force
    Expect-Check $false 'executavel ausente'
    Write-Host 'Verificador de distribuicao: 7/7 casos aprovados.'
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
