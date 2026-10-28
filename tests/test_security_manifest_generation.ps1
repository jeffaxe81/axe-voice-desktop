[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$Project=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Builder=Get-Content -LiteralPath (Join-Path $Project 'scripts/build-native-sdk-smoke.ps1') -Raw
$TailStart=$Builder.IndexOf('$Licenses =')
$Packager=Get-Content -LiteralPath (Join-Path $Project 'scripts/package-native-candidate.ps1') -Raw
$GateEnd=$Packager.IndexOf('# A repeated packaging run')
if($TailStart -lt 0 -or $GateEnd -lt 0){throw 'Manifest generation or package gate boundary missing'}
$Fixture=Join-Path ([IO.Path]::GetTempPath()) ('axe-manifest-generation-'+[guid]::NewGuid())
function Put([string]$Relative,[string]$Content){
 $Path=Join-Path $Fixture $Relative
 New-Item -ItemType Directory -Force (Split-Path $Path)|Out-Null
 [IO.File]::WriteAllText($Path,$Content)
}
function Get-Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
try{
 Put 'scripts/build-tail.ps1' $Builder.Substring($TailStart)
 Put 'scripts/gate.ps1' $Packager.Substring(0,$GateEnd)
 Put 'installer/axe-native-candidate.iss' "AppVersion=1.2.3`n"
 Put 'build/native-sdk-tls-media/product/CMakeCache.txt' "AXV_ENABLE_NATIVE_SIP:BOOL=ON`nAXV_NATIVE_TLS_MEDIA:BOOL=ON`nCMAKE_PROJECT_VERSION:STATIC=1.2.3`n"
 Put 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe' 'synthetic nonexecutable GUI'
 Put 'build/native-sdk-tls-media/prefix/include/a.h' 'synthetic header'
 Put 'deps/re/LICENSE' 'synthetic re license'
 Put 'deps/baresip/LICENSE' 'synthetic baresip license'
 Put 'deps/openssl/LICENSE.txt' 'synthetic openssl license'
 $Prefix=Join-Path $Fixture 'build/native-sdk-tls-media/prefix'
 $Re=Join-Path $Fixture 'deps/re';$Baresip=Join-Path $Fixture 'deps/baresip';$OpenSSL=Join-Path $Fixture 'deps/openssl'
 $TlsMedia=$true;$Flavor='tls-media';$Modules='g711;wasapi'
 $ProductVersion='1.2.3';$ProductHash=Get-Hash (Join-Path $Fixture 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe')
 $BaresipCommit=$ReCommit=$OpenSSLCommit='synthetic-commit'
 $HeaderHash=$LibraryHash=$ReHeaderHash=$ReLibraryHash='a'*64
 foreach($Case in @('clean','cached')){
  if($Case -eq 'cached'){[IO.File]::WriteAllText((Join-Path $Prefix 'README.txt'),'stale metadata from a previous build')}
  & (Join-Path $Fixture 'scripts/build-tail.ps1')
  $Manifest=Get-Content -LiteralPath (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') -Raw|ConvertFrom-Json -NoEnumerate
  $Expected=$Manifest.files.PSObject.Properties['README.txt'].Value
  if(!$Expected -or $Expected -ne (Get-Hash (Join-Path $Prefix 'README.txt'))){throw "SDK metadata missing or stale in $Case manifest"}
  & (Join-Path $Fixture 'scripts/gate.ps1')
 }
}finally{if(Test-Path -LiteralPath $Fixture){Remove-Item -LiteralPath $Fixture -Recurse -Force}}
Write-Output 'Fresh and cached SDK metadata are generated before manifest hashing'
