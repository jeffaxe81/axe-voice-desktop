[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$Run
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildDir = Join-Path $root 'build'
$binary = Join-Path $buildDir "$Configuration\AXE-Voice.exe"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake não encontrado. Instale CMake e abra um novo PowerShell.'
}
if (-not (Get-Command ctest -ErrorAction SilentlyContinue)) {
    throw 'CTest não encontrado. Verifique a instalação do CMake.'
}
if (-not (Get-Command 'git' -ErrorAction SilentlyContinue)) {
    Write-Warning 'Git não encontrado no PATH; compilação tentará continuar.'
}

Write-Host '[1/3] Configurando AXE Voice Windows x64...'
& cmake -S $root -B $buildDir -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw "Configuração CMake falhou ($LASTEXITCODE)." }

Write-Host '[2/3] Compilando...'
& cmake --build $buildDir --config $Configuration --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Compilação falhou ($LASTEXITCODE)." }

Write-Host '[3/3] Executando CTest...'
& ctest --test-dir $buildDir -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Testes falharam ($LASTEXITCODE)." }

if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
    throw "Executável não encontrado em: $binary"
}

Write-Host "AXE Voice compilado e testado: $binary"
Write-Warning 'Este é um protótipo de interface. SIP, áudio RTP/SRTP e registro real ainda não estão integrados.'
if ($Run) {
    & $binary
    if ($LASTEXITCODE -ne 0) { throw "AXE Voice saiu com código $LASTEXITCODE." }
}
