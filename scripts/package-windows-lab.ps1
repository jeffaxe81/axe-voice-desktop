[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration='Release')

$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cmakeFile=Join-Path $root 'CMakeLists.txt'
$matchLine=@(Get-Content -LiteralPath $cmakeFile | Where-Object {
    $_ -match '^project\(AXEVoiceDesktop VERSION [0-9]+\.[0-9]+\.[0-9]+ LANGUAGES CXX\)$'
})
if($matchLine.Count -ne 1) { throw 'Versao AXEVoiceDesktop ausente ou ambigua no CMake.' }
$versionMatch=[regex]::Match($matchLine[0], 'VERSION ([0-9]+\.[0-9]+\.[0-9]+) ')
if(-not $versionMatch.Success) { throw 'Versao AXEVoiceDesktop invalida no CMake.' }
$version=$versionMatch.Groups[1].Value
$binary=Join-Path $root "build/$Configuration/AXE-Voice.exe"
if(-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
    throw "Executavel Windows nao encontrado: $binary"
}
$stage=Join-Path $root 'build/AXE-Voice-Windows-Lab-Stage'
$dist=Join-Path $root 'dist'
$zip=Join-Path $dist "AXE-Voice-Windows-Lab-v$version.zip"
if(Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null
New-Item -ItemType Directory -Path $dist -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination (Join-Path $stage 'AXE-Voice.exe') -ErrorAction Stop
$sha=(Get-FileHash -LiteralPath (Join-Path $stage 'AXE-Voice.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
$readme=@"
AXE VOICE DESKTOP - LABORATORIO / NAO OPERACIONAL
Versao: $version
Plataforma: Windows x64

ATENCAO: Esta versao e um prototipo visual. Nao registra ramais SIP,
nao realiza ou recebe chamadas e nao possui audio RTP/SRTP operacional.
NAO use em producao. NAO configure senhas reais.

Como abrir: extraia este ZIP em uma pasta e execute AXE-Voice.exe.
O executavel nao e assinado digitalmente nem possui instalador.
O Windows pode mostrar avisos de origem desconhecida.

Integridade: compare o SHA-256 de AXE-Voice.exe com SHA256SUMS.txt.
PowerShell: (Get-FileHash .\AXE-Voice.exe -Algorithm SHA256).Hash
Verificar o hash apenas detecta modificacao; nao autentica a origem.
"@
Set-Content -LiteralPath (Join-Path $stage 'README.txt') -Value $readme -Encoding utf8
Set-Content -LiteralPath (Join-Path $stage 'SHA256SUMS.txt') -Value "$sha  AXE-Voice.exe" -Encoding ascii
if(Test-Path -LiteralPath $zip) {
    Remove-Item -LiteralPath $zip -Force
}
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
if(-not (Test-Path -LiteralPath $zip -PathType Leaf)) {
    throw "Falha ao montar pacote $zip"
}
Write-Host "Pacote Windows LABORATORIO criado: $zip"
Write-Host "SHA256 executavel: $sha"
Write-Warning 'NAO OPERACIONAL: sem SIP real, chamadas ou RTP/SRTP.'
