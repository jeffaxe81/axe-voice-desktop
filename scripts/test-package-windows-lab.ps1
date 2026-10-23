[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration='Release')

$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cmakeLine=@(Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') | Where-Object {
    $_ -match '^project\(AXEVoiceDesktop VERSION [0-9]+\.[0-9]+\.[0-9]+ LANGUAGES CXX\)$'
})
if($cmakeLine.Count -ne 1) { throw 'Versao unica nao encontrada no CMake' }
$version=[regex]::Match($cmakeLine[0],'VERSION ([0-9]+\.[0-9]+\.[0-9]+) ').Groups[1].Value
$zip=Join-Path $root "dist/AXE-Voice-Windows-Lab-v$version.zip"
$builtExe=Join-Path $root "build/$Configuration/AXE-Voice.exe"
if(-not (Test-Path -LiteralPath $zip -PathType Leaf)) { throw "ZIP nao existe: $zip" }
if(-not (Test-Path -LiteralPath $builtExe -PathType Leaf)) { throw 'Executavel do build nao encontrado' }
Add-Type -AssemblyName System.IO.Compression
$builtHash=(Get-FileHash -LiteralPath $builtExe -Algorithm SHA256).Hash.ToLowerInvariant()

function Assert-LabPackage([string]$archivePath) {
    $archive=[System.IO.Compression.ZipFile]::OpenRead($archivePath)
    try {
        $expected=@('AXE-Voice.exe','README.txt','SHA256SUMS.txt')
        $actual=@($archive.Entries | ForEach-Object { $_.FullName })
        if($actual.Count -ne $expected.Count) {
            throw "O ZIP deve ter somente tres arquivos, encontrou $($actual.Count)"
        }
        foreach($file in $expected) {
            if(@($actual | Where-Object { $_ -ceq $file }).Count -ne 1) {
                throw "Arquivo do pacote ausente ou duplicado: $file"
            }
        }
        # Exact allowlisting prevents shipping accidental settings, SDK, logs,
        # passwords, unknown DLLs or path-traversal entries.
        $checksumEntry=$archive.GetEntry('SHA256SUMS.txt')
        $reader=[System.IO.StreamReader]::new($checksumEntry.Open(),[System.Text.Encoding]::ASCII)
        try { $manifest=$reader.ReadToEnd().Trim() } finally { $reader.Dispose() }
        $match=[regex]::Match($manifest,'^([0-9a-f]{64})  AXE-Voice\.exe$')
        if(-not $match.Success) { throw 'Manifesto SHA256SUMS.txt incorreto' }
        $declaredHash=$match.Groups[1].Value

        $exeStream=$archive.GetEntry('AXE-Voice.exe').Open()
        $sha=[System.Security.Cryptography.SHA256]::Create()
        try {
            $bytes=$sha.ComputeHash($exeStream)
            $archiveHash=[System.BitConverter]::ToString($bytes).Replace('-','').ToLowerInvariant()
        } finally {
            $sha.Dispose()
            $exeStream.Dispose()
        }
        if($archiveHash -cne $declaredHash -or $archiveHash -cne $builtHash) {
            throw 'SHA-256 diferente entre executavel, manifesto e build'
        }
        $readmeStream=$archive.GetEntry('README.txt').Open()
        $readmeReader=[System.IO.StreamReader]::new($readmeStream,[System.Text.Encoding]::UTF8)
        try { $readme=$readmeReader.ReadToEnd() } finally { $readmeReader.Dispose() }
        if($readme -notmatch 'NAO OPERACIONAL' -or
           $readme -notmatch [regex]::Escape("Versao: $version") -or
           $readme -notmatch 'NAO use em producao') {
            throw 'README de laboratorio sem aviso ou versao'
        }
    } finally {
        $archive.Dispose()
    }
}

function Expect-Rejection([string]$archivePath,[string]$caseName) {
    $rejected=$false
    try { Assert-LabPackage $archivePath } catch { $rejected=$true }
    if(-not $rejected) { throw "Teste negativo '$caseName' aceitou pacote adulterado" }
}

Assert-LabPackage $zip
$tempDir=Join-Path $root 'build/axv-package-negative-tests'
if(Test-Path -LiteralPath $tempDir) { Remove-Item -LiteralPath $tempDir -Force -Recurse }
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
try {
    $tampered=Join-Path $tempDir 'tampered-checksum.zip'
    Copy-Item -LiteralPath $zip -Destination $tampered
    $archive=[System.IO.Compression.ZipFile]::Open($tampered,[System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $entry=$archive.GetEntry('SHA256SUMS.txt')
        $entry.Delete()
        $entry=$archive.CreateEntry('SHA256SUMS.txt')
        $writer=[System.IO.StreamWriter]::new($entry.Open(),[System.Text.Encoding]::ASCII)
        try { $writer.Write(('0'*64)+'  AXE-Voice.exe') } finally { $writer.Dispose() }
    } finally { $archive.Dispose() }
    Expect-Rejection $tampered 'checksum errado'

    $unexpected=Join-Path $tempDir 'unexpected-content.zip'
    Copy-Item -LiteralPath $zip -Destination $unexpected
    $archive=[System.IO.Compression.ZipFile]::Open($unexpected,[System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $extra=$archive.CreateEntry('settings/secret.env')
        $writer=[System.IO.StreamWriter]::new($extra.Open(),[System.Text.Encoding]::ASCII)
        try { $writer.Write('dummy-test-only') } finally { $writer.Dispose() }
    } finally { $archive.Dispose() }
    Expect-Rejection $unexpected 'conteudo inesperado'
} finally {
    if(Test-Path -LiteralPath $tempDir) { Remove-Item -LiteralPath $tempDir -Force -Recurse }
}
Write-Host 'PACOTE LABORATORIO: verificado, hash confirmado, 2 adulteracoes rejeitadas.'
