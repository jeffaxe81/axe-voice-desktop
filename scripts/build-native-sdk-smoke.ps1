[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Work = Join-Path $Root 'build/native-sdk-deps'
$Prefix = Join-Path $Root 'build/native-sdk-prefix'
$ProductBuild = Join-Path $Root 'build/native-sdk-product'
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
function Get-PinnedSource([string]$Name, [string]$Repository, [string]$Commit) {
    $Destination = Join-Path $Work $Name
    if (!(Test-Path (Join-Path $Destination '.git'))) {
        New-Item -ItemType Directory -Force $Destination | Out-Null
        Invoke-Checked 'git' @('-C',$Destination,'init')
        Invoke-Checked 'git' @('-C',$Destination,'remote','add','origin',$Repository)
        Invoke-Checked 'git' @('-C',$Destination,'fetch','--depth','1','origin',$Commit)
        Invoke-Checked 'git' @('-C',$Destination,'checkout','--detach',$Commit)
    }
    $Actual = (& git -C $Destination rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $Actual -ne $Commit) { throw "Unexpected SDK revision: $Name" }
    $Dirty = & git -C $Destination status --porcelain --untracked-files=no
    if ($LASTEXITCODE -ne 0 -or $Dirty) { throw "SDK tracked files modified: $Name" }
    return $Destination
}
$ReCommit = '65555dad439de9d6a81b3ee6092b9c096585249b'
$BaresipCommit = '9cfab81550acc4bd38d6580d79b45cc6eea65cd6'
New-Item -ItemType Directory -Force $Work,$Prefix | Out-Null
$Re = Get-PinnedSource 're' 'https://github.com/baresip/re.git' $ReCommit
$Baresip = Get-PinnedSource 'baresip' 'https://github.com/baresip/baresip.git' $BaresipCommit
$Common = @('-G','Visual Studio 17 2022','-A','x64',"-DCMAKE_INSTALL_PREFIX=$Prefix",
    "-DCMAKE_PREFIX_PATH=$Prefix",'-DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=ON',
    '-DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=ON','-DUSE_OPENSSL=OFF','-DUSE_MBEDTLS=OFF')
$ReBuild = Join-Path $Work 're-build'
Invoke-Checked 'cmake' (@('-S',$Re,'-B',$ReBuild) + $Common + @(
    '-DLIBRE_BUILD_SHARED=OFF','-DLIBRE_BUILD_STATIC=ON','-DUSE_BFCP=OFF','-DUSE_PCP=OFF','-DUSE_RTMP=OFF'))
Invoke-Checked 'cmake' @('--build',$ReBuild,'--config','Release','--parallel','4')
Invoke-Checked 'cmake' @('--install',$ReBuild,'--config','Release')
$BsBuild = Join-Path $Work 'baresip-build'
Invoke-Checked 'cmake' (@('-S',$Baresip,'-B',$BsBuild) + $Common + @('-DSTATIC=ON','-DMODULES:STRING='))
Invoke-Checked 'cmake' @('--build',$BsBuild,'--config','Release','--parallel','4')
Invoke-Checked 'cmake' @('--install',$BsBuild,'--config','Release')
$Header = Join-Path $Prefix 'include/baresip.h'
$Library = Join-Path $Prefix 'lib/libbaresip.lib'
$ReHeader = Join-Path $Prefix 'include/re/re.h'
$ReLibrary = Join-Path $Prefix 'lib/re-static.lib'
function Get-Hash([string]$Path) { return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
$HeaderHash = Get-Hash $Header
$LibraryHash = Get-Hash $Library
$ReHeaderHash = Get-Hash $ReHeader
$ReLibraryHash = Get-Hash $ReLibrary
Invoke-Checked 'cmake' @('-S',$Root,'-B',$ProductBuild,'-G','Visual Studio 17 2022','-A','x64',
    '-DBUILD_TESTING=OFF','-DAXV_BUILD_NATIVE_BARESIP_ADAPTER=ON',
    "-DAXV_BARESIP_INCLUDE_DIR=$Prefix/include","-DAXV_BARESIP_LIBRARY=$Library",
    "-DAXV_BARESIP_HEADER_SHA256=$HeaderHash","-DAXV_BARESIP_LIBRARY_SHA256=$LibraryHash",
    '-DAXV_BARESIP_VERSION=4.12.0',"-DAXV_LIBRE_INCLUDE_DIR=$Prefix/include/re",
    "-DAXV_LIBRE_LIBRARY=$ReLibrary","-DAXV_LIBRE_HEADER_SHA256=$ReHeaderHash",
    "-DAXV_LIBRE_LIBRARY_SHA256=$ReLibraryHash")
Invoke-Checked 'cmake' @('--build',$ProductBuild,'--config','Release','--parallel','4','--target',
    'axv_native_baresip_sdk_abi_probe','axv_native_baresip_abi_probe','axv_native_libre_reactor_smoke')
# The ABI executables are build-only. Execute only the local libre reactor;
# it creates no Baresip UA, account, SIP registration or media session.
Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_libre_reactor_smoke.exe') @()
$Licenses = Join-Path $Prefix 'licenses'
New-Item -ItemType Directory -Force $Licenses | Out-Null
Copy-Item (Join-Path $Re 'LICENSE') (Join-Path $Licenses 'libre-LICENSE.txt')
Copy-Item (Join-Path $Baresip 'LICENSE') (Join-Path $Licenses 'baresip-LICENSE.txt')
@{ profile='MSVC x64 ABI/reactor candidate only'; tls=$false; audio_modules=@(); gui_native_sdk='OFF/not linked';
   baresip_commit=$BaresipCommit; libre_commit=$ReCommit; version='4.12.0';
   baresip_header_sha256=$HeaderHash; baresip_library_sha256=$LibraryHash;
   libre_header_sha256=$ReHeaderHash; libre_library_sha256=$ReLibraryHash;
   reactor_smoke='exit 0'; production_approved=$false } |
    ConvertTo-Json -Depth 4 | Set-Content (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') -Encoding utf8
'Candidate SDK for ABI and reactor checks. TLS/audio modules disabled. No SIP or production approval.' |
    Set-Content (Join-Path $Prefix 'README.txt') -Encoding utf8
