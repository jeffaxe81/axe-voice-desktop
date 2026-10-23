# AXE Voice Windows LAB only. Compares a local binary against its own
# SHA-256 manifest. It does NOT verify publisher identity or sign the build.
[CmdletBinding()]
param([string]$Directory = (Get-Location).Path)

$ErrorActionPreference = 'Stop'
$folder = (Resolve-Path -LiteralPath $Directory -ErrorAction Stop).Path
$binary = Join-Path $folder 'AXE-Voice.exe'
$manifest = Join-Path $folder 'AXE-Voice-LAB.sha256'
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
    throw 'Executavel AXE-Voice.exe ausente.'
}
if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
    throw 'Manifesto AXE-Voice-LAB.sha256 ausente.'
}

$content = Get-Content -LiteralPath $manifest -Encoding utf8 -Raw
$content = $content -replace "`r`n", "`n"
if ($content -cnotmatch '\A([0-9a-fA-F]{64})  AXE-Voice\.exe\n\z') {
    throw 'Manifesto invalido: requer uma unica linha SHA-256 com nome exato.'
}
$expected = $Matches[1].ToLowerInvariant()
$actual = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -cne $expected) {
    throw 'INTEGRIDADE FALHOU: arquivo diferente do SHA-256 do manifesto.'
}
Write-Host 'Integridade SHA-256 local: OK (AXE-Voice.exe).'
Write-Warning 'Manifesto e binario no mesmo pacote nao comprovam autoria, assinatura digital ou funcionamento SIP.'
