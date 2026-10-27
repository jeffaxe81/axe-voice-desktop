[CmdletBinding()]
param([switch]$TlsMedia)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Flavor = if ($TlsMedia) { 'tls-media' } else { 'abi' }
$Work = Join-Path $Root "build/native-sdk-deps-$Flavor"
$Prefix = Join-Path $Root "build/native-sdk-$Flavor/prefix"
$ProductBuild = Join-Path $Root "build/native-sdk-$Flavor/product"
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
$OpenSSLCommit = '8cf17aaeb4599f8af87fefd810b5b5fee90fe69e'
if ($TlsMedia) {
    $Vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $VsRoot = (& $Vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
    if ($LASTEXITCODE -ne 0 -or !$VsRoot) { throw 'MSVC development environment not found' }
    $DevCmd = Join-Path $VsRoot 'Common7/Tools/VsDevCmd.bat'
    # A batch file avoids nested cmd/PowerShell quoting of Program Files paths.
    $ImportTools = Join-Path $Work 'import-msvc.cmd'
    @('@echo off', ('call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul' -f $DevCmd),
      'if errorlevel 1 exit /b 1', 'set') | Set-Content $ImportTools -Encoding ascii
    $ToolEnvironment = & cmd.exe /d /c $ImportTools
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize MSVC x64 tools' }
    foreach ($Line in $ToolEnvironment) {
        if ($Line -match '^([^=]+)=(.*)$') {
            $Name = $Matches[1]; $Value = $Matches[2]
            if ($Name -match '^(PATH|INCLUDE|LIB|LIBPATH|VCTools.*|WindowsSdk.*|VSINSTALLDIR|VCINSTALLDIR|UniversalCRTSdkDir|UCRTVersion|VSCMD.*|DevEnvDir)$') {
                [Environment]::SetEnvironmentVariable($Name,$Value,'Process')
            }
        }
    }
    $OpenSSL = Get-PinnedSource 'openssl' 'https://github.com/openssl/openssl.git' $OpenSSLCommit
    Push-Location $OpenSSL
    try {
        $ConfigureArguments=@('Configure','VC-WIN64A','shared','no-tests','no-module','no-asm',"--prefix=$Prefix",'--libdir=lib',"--openssldir=$Prefix/ssl")
        # Reuse only a cache whose actual OpenSSL configuration matches this
        # pinned target/profile. Re-running Configure touches every dependency.
        $Reuse=$false
        if(Test-Path 'configdata.pm') {
            $Inspect='my $want=shift; my $got=$configdata::config{prefix}; $want=~s!\\!/!g; $got=~s!\\!/!g; exit(($configdata::config{target} eq "VC-WIN64A" && $got eq $want && $configdata::config{libdir} eq "lib" && exists $configdata::disabled{asm} && exists $configdata::disabled{tests} && exists $configdata::disabled{module} && !exists $configdata::disabled{shared}) ? 0 : 1);'
            & perl '-I.' '-Mconfigdata' '-e' $Inspect $Prefix
            $Reuse=$LASTEXITCODE -eq 0
        }
        if(!$Reuse) { Invoke-Checked 'perl' $ConfigureArguments }
        Invoke-Checked 'nmake' @('/NOLOGO')
        Invoke-Checked 'nmake' @('/NOLOGO','install_sw')
    } finally { Pop-Location }
    $env:PATH = "$Prefix/bin;$env:PATH"
}
$Common = @('-G','Visual Studio 17 2022','-A','x64',"-DCMAKE_INSTALL_PREFIX=$Prefix",
    "-DCMAKE_PREFIX_PATH=$Prefix",'-DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=ON','-DUSE_MBEDTLS=OFF')
if ($TlsMedia) {
    $Common += @('-DUSE_OPENSSL=ON',"-DOPENSSL_ROOT_DIR=$Prefix",'-DOPENSSL_USE_STATIC_LIBS=OFF')
} else {
    $Common += @('-DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=ON','-DUSE_OPENSSL=OFF')
}
$ReBuild = Join-Path $Work 're-build'
Invoke-Checked 'cmake' (@('-S',$Re,'-B',$ReBuild) + $Common + @(
    '-DLIBRE_BUILD_SHARED=OFF','-DLIBRE_BUILD_STATIC=ON','-DUSE_BFCP=OFF','-DUSE_PCP=OFF','-DUSE_RTMP=OFF'))
Invoke-Checked 'cmake' @('--build',$ReBuild,'--config','Release','--parallel','4')
Invoke-Checked 'cmake' @('--install',$ReBuild,'--config','Release')
$BsBuild = Join-Path $Work 'baresip-build'
$Modules = if ($TlsMedia) { 'g711;wasapi' } else { '' }
Invoke-Checked 'cmake' (@('-S',$Baresip,'-B',$BsBuild) + $Common + @('-DSTATIC=ON',"-DMODULES:STRING=$Modules"))
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
$ProductArguments = @('-S',$Root,'-B',$ProductBuild,'-G','Visual Studio 17 2022','-A','x64',
    '-DBUILD_TESTING=OFF','-DAXV_BUILD_NATIVE_BARESIP_ADAPTER=ON',
    "-DAXV_BARESIP_INCLUDE_DIR=$Prefix/include","-DAXV_BARESIP_LIBRARY=$Library",
    "-DAXV_BARESIP_HEADER_SHA256=$HeaderHash","-DAXV_BARESIP_LIBRARY_SHA256=$LibraryHash",
    '-DAXV_BARESIP_VERSION=4.12.0',"-DAXV_LIBRE_INCLUDE_DIR=$Prefix/include/re",
    "-DAXV_LIBRE_LIBRARY=$ReLibrary","-DAXV_LIBRE_HEADER_SHA256=$ReHeaderHash",
    "-DAXV_LIBRE_LIBRARY_SHA256=$ReLibraryHash")
if ($TlsMedia) { $ProductArguments += @('-DAXV_NATIVE_TLS_MEDIA=ON','-DAXV_ENABLE_NATIVE_SIP=ON',"-DOPENSSL_ROOT_DIR=$Prefix",'-DOPENSSL_USE_STATIC_LIBS=OFF') }
Invoke-Checked 'cmake' $ProductArguments
Invoke-Checked 'cmake' @('--build',$ProductBuild,'--config','Release','--parallel','4','--target',
    'axv_native_baresip_sdk_abi_probe','axv_native_baresip_abi_probe','axv_native_libre_reactor_smoke')
if ($TlsMedia) {
    Invoke-Checked 'cmake' @('--build',$ProductBuild,'--config','Release','--parallel','4','--target','axv_native_tls_media_probe')
    Get-ChildItem (Join-Path $Prefix 'bin') -Filter '*.dll' | Copy-Item -Destination (Join-Path $ProductBuild 'Release')
    Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_tls_media_probe.exe') @()
    Invoke-Checked 'cmake' @('--build',$ProductBuild,'--config','Release','--parallel','4','--target','axv_native_credential_lifecycle_smoke','axv_native_pending_secret_smoke','axv_native_sip_engine_smoke','axv_native_sdk_secret_smoke','axv_native_audio_config_smoke','axv_native_audio_mute_smoke','AXEVoice')
    Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_sip_engine_smoke.exe') @()
    foreach ($Mode in 1..5) { Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_credential_lifecycle_smoke.exe') @([string]$Mode) }
    Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_pending_secret_smoke.exe') @()
    Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_sdk_secret_smoke.exe') @()
    Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_audio_config_smoke.exe') @()
    Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_audio_mute_smoke.exe') @()
    Invoke-Checked 'python' @((Join-Path $Root 'tests/run_native_sip_loopback.py'),(Join-Path $ProductBuild 'Release/axv_native_sip_engine_smoke.exe'))
    Invoke-Checked (Join-Path $ProductBuild 'Release/AXE-Voice.exe') @('--native-ui-smoke')
}
# The ABI executables are build-only. Also execute the local libre reactor;
# it creates no Baresip UA, account, SIP registration or media session.
Invoke-Checked (Join-Path $ProductBuild 'Release/axv_native_libre_reactor_smoke.exe') @()
# Bind packaging to the exact GUI binary that passed the smoke above.
$ProductCache = Get-Content (Join-Path $ProductBuild 'CMakeCache.txt') -Raw
$ProductVersion = [regex]::Match($ProductCache, '(?m)^CMAKE_PROJECT_VERSION:STATIC=([^\r\n]+)\r?$').Groups[1].Value
if (!$ProductVersion) { throw 'Product version missing from CMake cache' }
$ProductHash = if ($TlsMedia) { Get-Hash (Join-Path $ProductBuild 'Release/AXE-Voice.exe') } else { $null }
$Licenses = Join-Path $Prefix 'licenses'
New-Item -ItemType Directory -Force $Licenses | Out-Null
Copy-Item (Join-Path $Re 'LICENSE') (Join-Path $Licenses 'libre-LICENSE.txt')
Copy-Item (Join-Path $Baresip 'LICENSE') (Join-Path $Licenses 'baresip-LICENSE.txt')
if ($TlsMedia) { Copy-Item (Join-Path $OpenSSL 'LICENSE.txt') (Join-Path $Licenses 'OpenSSL-LICENSE.txt') }
$Files = @{}
Get-ChildItem $Prefix -Recurse -File | Where-Object { $_.Extension -in @('.h','.lib','.dll') } | ForEach-Object {
    $Files[$_.FullName.Substring($Prefix.Length+1).Replace('\','/')] = Get-Hash $_.FullName
}
@{ product_version=$ProductVersion; product_sha256=$ProductHash; profile="MSVC x64 $Flavor candidate only"; tls=[bool]$TlsMedia; audio_modules=@($Modules -split ';' | Where-Object { $_ }); gui_native_sdk=$(if ($TlsMedia) { 'ON/native candidate' } else { 'OFF/not linked' });
   baresip_commit=$BaresipCommit; libre_commit=$ReCommit; version='4.12.0';
   baresip_header_sha256=$HeaderHash; baresip_library_sha256=$LibraryHash;
   libre_header_sha256=$ReHeaderHash; libre_library_sha256=$ReLibraryHash;
   reactor_smoke='exit 0'; tls_media_probe=$(if ($TlsMedia) { 'exit 0, no handshake/devices opened' } else { 'not applicable' }); openssl_commit=$(if ($TlsMedia) { $OpenSSLCommit } else { $null }); files=$Files; production_approved=$false } |
    ConvertTo-Json -Depth 4 | Set-Content (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') -Encoding utf8
"Candidate SDK profile: $Flavor. TLS/media compilation is not handshake, device, call or production approval." |
    Set-Content (Join-Path $Prefix 'README.txt') -Encoding utf8
