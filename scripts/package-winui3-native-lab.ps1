# Build and verify the WinUI 3 native LAB installer without starting SIP.
# No credentials, tokens, registered accounts or calls are required.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source=Join-Path $root 'build/winui3-native-candidate'
$native=Join-Path $root 'build/native-sdk-tls-media/product/Release/axv_frontend_bridge.dll'
$prefix=Join-Path $root 'build/native-sdk-tls-media/prefix'
$output=Join-Path $root 'build/winui3-native-lab-installer'
$target=Join-Path $root 'build/winui3-native-lab-install-check'
$spec=Join-Path $root 'installer/axe-winui3-native-lab.iss'
$setupLog=Join-Path $output 'winui3-install-lab.log'

function Require-File([string]$path) {
    if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing laboratory distribution file: $path" }
}
function Hash([string]$path) {
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
}
foreach($relative in @(
    'AXEVoice.WinUI3.exe','axv_frontend_bridge.dll','Assets/axe-voice.ico',
    'LEIA-ME-CANDIDATO.txt','licenses/baresip-LICENSE.txt',
    'licenses/libre-LICENSE.txt','licenses/OpenSSL-LICENSE.txt'
)) { Require-File (Join-Path $source $relative) }
Require-File $native
Require-File (Join-Path $prefix 'CANDIDATE-MANIFEST.json')
if((Hash (Join-Path $source 'axv_frontend_bridge.dll')) -ne (Hash $native)) {
    throw 'WinUI 3 package bridge is not the pinned native SIP build'
}
# The installed program remains explicitly a LAB candidate. Generic WinUI
# Preview bridges and SDK-free archives are not accepted here.
$manifest=Get-Content -LiteralPath (Join-Path $prefix 'CANDIDATE-MANIFEST.json') -Raw |
    ConvertFrom-Json -NoEnumerate
if($manifest.gui_native_sdk -ne 'ON/native candidate' -or
   $manifest.tls -ne $true -or $manifest.production_approved -ne $false) {
    throw 'Native SDK manifest does not authorize a WinUI 3 LAB candidate'
}
foreach($dll in @(Get-ChildItem -LiteralPath (Join-Path $prefix 'bin') -Filter '*.dll' -File)) {
    $copy=Join-Path $source $dll.Name
    Require-File $copy
    if((Hash $copy) -ne (Hash $dll.FullName)) {
        throw "WinUI 3 distribution DLL hash mismatch: $($dll.Name)"
    }
}
$cmake=Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw
$version=[regex]::Match($cmake,'(?m)^project\(AXEVoiceDesktop VERSION ([0-9]+\.[0-9]+\.[0-9]+) ').Groups[1].Value
$iss=Get-Content -LiteralPath $spec -Raw
$issVersion=[regex]::Match($iss,'(?m)^AppVersion=([^\r\n]+)').Groups[1].Value
if(!$version -or $version -ne $issVersion){throw 'WinUI 3 installer and CMake product versions differ'}
if($iss -notmatch '(?m)^AppId=AXEVoiceWinUI3NativeLabCandidate\s*$' -or
   $iss -notmatch '(?m)^PrivilegesRequired=lowest\s*$' -or
   $iss -notmatch '(?m)^Name: "desktopicon";.*Flags: unchecked\s*$') {
    throw 'WinUI 3 native lab installer isolation, per-user access, or optional shortcut invalid'
}
if($iss -match '(?m)^\[Run\]') {
    throw 'WinUI 3 laboratory installer must not auto-launch/register SIP'
}
$compiler=Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'
Require-File $compiler
New-Item -ItemType Directory -Force $output | Out-Null
& $compiler "/DWinUI3Source=$source" "/DWinUI3Output=$output" $spec
if($LASTEXITCODE -ne 0) {throw 'WinUI 3 native LAB Inno setup compilation failed'}
$installer=Join-Path $output 'AXE-Voice-WinUI3-Native-Lab-Setup.exe'
Require-File $installer
$desktop=[Environment]::GetFolderPath('DesktopDirectory')
if(!$desktop){throw 'Could not resolve interactive current-user Desktop folder'}
$link=Join-Path $desktop 'AXE Voice WinUI 3 Native Lab.lnk'
if(Test-Path -LiteralPath $link){throw 'WinUI 3 lab test requires no prior desktop shortcut'}
function Wait-ProcessBounded([string]$file,[string[]]$arguments,[string]$stage,[int]$seconds=120) {
    # Inno Setup may display its startup prompt even with /VERYSILENT.
    # /SP- disables it; /LOG provides evidence of actual setup execution.
    if (!($arguments -contains '/VERYSILENT') -or
        !($arguments -contains '/SUPPRESSMSGBOXES') -or
        !($arguments -contains '/NORESTART')) {
        throw "$stage requires explicit silent switches"
    }
    # Inno setup supports /SP- for the install startup prompt; the
    # uninstaller has no startup prompt and does not require those flags.
    if ($stage -eq 'WinUI 3 lab install' -and
        (!($arguments -contains '/SP-') -or
         @($arguments | Where-Object { $_ -like '/LOG=*' }).Count -ne 1)) {
        throw "$stage requires startup suppression and a dedicated trace"
    }
    # Start-Process -Wait waits for the entire descendant process tree and may
    # block forever on a separate Inno child. Await the original process
    # handle instead, then verify the actual installed payload and uninstaller.
    $process=Start-Process -FilePath $file -ArgumentList $arguments -PassThru
    try {
        if(!$process.WaitForExit($seconds*1000)) {
            throw "$stage did not finish within $seconds seconds (PID $($process.Id)). Installer left for manual runner inspection; no process was killed."
        }
        if($process.ExitCode -ne 0) {
            throw "$stage returned nonzero code $($process.ExitCode)"
        }
    } finally {$process.Dispose()}
}
function Invoke-Setup([string[]]$SetupArguments) {
    # Do not declare a parameter called $args: it is a PowerShell automatic
    # variable and may shadow the caller-provided silent install switches.
    Wait-ProcessBounded -file $installer -arguments $SetupArguments -stage 'WinUI 3 lab install'
}
function Get-LabUninstaller {
    # Inno uses unins000.exe normally, but an existing installation may
    # legitimately produce unins001.exe or another numbered suffix.
    $matches=@(Get-ChildItem -LiteralPath $target -File -Filter 'unins*.exe' -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^unins[0-9]{3}\.exe
    # Inno's first bootstrap process may return before its setup.tmp child has
    # finished installing. Verify actual files AND require that both known
    # installer processes have exited, instead of treating parent exit 0 as
    # proof that the application was deployed.
    $deadline=[DateTime]::UtcNow.AddSeconds($seconds)
    $installedExe=Join-Path $target 'AXEVoice.WinUI3.exe'
    $activeCount=0
    do {
        $active=@(Get-CimInstance Win32_Process -Filter "Name LIKE 'AXE-Voice-WinUI3-Native-Lab-Setup%'")
        $activeCount=$active.Count
        if((Get-LabUninstaller) -and
           (Test-Path -LiteralPath $installedExe -PathType Leaf) -and
           $activeCount -eq 0) {
            return
        }
        Start-Sleep -Milliseconds 400
    } while([DateTime]::UtcNow -lt $deadline)
    $default=Join-Path $env:LOCALAPPDATA 'Programs/AXEVoiceWinUI3NativeLab/unins000.exe'
    $wrongLocation=Test-Path -LiteralPath $default -PathType Leaf
    $actualExe=Test-Path -LiteralPath $installedExe -PathType Leaf
    $actualUninstaller=[bool](Get-LabUninstaller)
    $uninstallerCandidates=@(Get-ChildItem -LiteralPath $target -Filter 'unins*.exe' -File -ErrorAction SilentlyContinue)
    $uninstallerData=@(Get-ChildItem -LiteralPath $target -Filter 'unins*.dat' -File -ErrorAction SilentlyContinue)
    $setupLogExists=Test-Path -LiteralPath $setupLog -PathType Leaf
    $traceSize=if($setupLogExists){(Get-Item -LiteralPath $setupLog).Length}else{0}
    # Do not dump raw Inno logs into public PR CI: they may contain user
    # filesystem paths or other environment data. Emit existence and sizes.
    throw "WinUI 3 lab installation incomplete: exe=$actualExe, pairedUninstaller=$actualUninstaller, uninstallerExeCount=$($uninstallerCandidates.Count), uninstallerDatCount=$($uninstallerData.Count), activeSetup=$activeCount, defaultDirectoryHasUninstaller=$wrongLocation, setupLogExists=$setupLogExists, setupLogBytes=$traceSize. No process was killed."
}
function Verify-Payload {
    foreach($file in @(Get-ChildItem -LiteralPath $source -Recurse -File)) {
        $relative=$file.FullName.Substring($source.Length+1)
        $installed=Join-Path $target $relative
        Require-File $installed
        if((Hash $file.FullName) -ne (Hash $installed)) {
            throw "Installed WinUI 3 lab file hash mismatch: $relative"
        }
    }
}
function Invoke-Uninstall {
    $uninstaller=Get-LabUninstaller
    if(!$uninstaller) {throw 'A unique matching Inno uninstall EXE/DAT pair was not found'}
    Require-File $uninstaller
    Wait-ProcessBounded $uninstaller @(
        '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
    ) 'WinUI 3 lab uninstall'
    # The Inno uninstaller can also spawn a helper which outlives the initial
    # bootstrap handle. Require the real installed payload to be gone before
    # asserting success. Never delete any files or terminate unknown processes.
    $deadline=[DateTime]::UtcNow.AddSeconds(120)
    $remaining=@()
    do {
        $remaining=@(Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object {
            $relative=$_.FullName.Substring($source.Length+1)
            Test-Path -LiteralPath (Join-Path $target $relative)
        })
        $active=@(Get-CimInstance Win32_Process -Filter "Name LIKE 'unins000%'")
        if($remaining.Count -eq 0 -and $active.Count -eq 0 -and
           !(Test-Path -LiteralPath $link)) { return }
        Start-Sleep -Milliseconds 400
    } while([DateTime]::UtcNow -lt $deadline)
    throw "WinUI 3 lab uninstall incomplete: remainingPayload=$($remaining.Count), activeUninstaller=$($active.Count), desktopShortcutExists=$(Test-Path -LiteralPath $link)"

}
$common=@('/SP-','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',
          "/DIR=`"$target`"","/LOG=`"$setupLog`"")
try {
    Invoke-Setup $common
    Wait-InstalledCompletion
    Verify-Payload
    if(Test-Path -LiteralPath $link){throw 'Default installation unexpectedly created a desktop shortcut'}
    Invoke-Uninstall
    # Explicit user opt-in should create the correct executable link and
    # remove it on uninstall, without launching or connecting the softphone.
    Invoke-Setup ($common + @('/TASKS=desktopicon'))
    Wait-InstalledCompletion
    Verify-Payload
    Require-File $link
    $shell=New-Object -ComObject WScript.Shell
    $shortcut=$shell.CreateShortcut($link)
    try {
        if(![string]::Equals([IO.Path]::GetFullPath($shortcut.TargetPath),
             [IO.Path]::GetFullPath((Join-Path $target 'AXEVoice.WinUI3.exe')),
             [StringComparison]::OrdinalIgnoreCase)) {
            throw 'WinUI 3 desktop shortcut target differs from the installed app'
        }
    } finally {
        if($shortcut){[void][Runtime.InteropServices.Marshal]::ReleaseComObject($shortcut)}
        if($shell){[void][Runtime.InteropServices.Marshal]::ReleaseComObject($shell)}
    }
    Invoke-Uninstall
} finally {
    # Never kill unknown processes, delete user files, or remove a shortcut
    # whose identity was not proved. Failure remains visible to CI.
}
Write-Host "WinUI3 native LAB installer PASS: default off, opt-in shortcut, content hashes, uninstall, version $version."
Get-FileHash -LiteralPath $installer -Algorithm SHA256 | Format-List
 -and
            (Test-Path -LiteralPath (Join-Path $target ($_.BaseName+'.dat')) -PathType Leaf) })
    if($matches.Count -eq 1) { return $matches[0].FullName }
    return $null
}
function Wait-InstalledCompletion([int]$seconds=150) {
    # Inno's first bootstrap process may return before its setup.tmp child has
    # finished installing. Verify actual files AND require that both known
    # installer processes have exited, instead of treating parent exit 0 as
    # proof that the application was deployed.
    $deadline=[DateTime]::UtcNow.AddSeconds($seconds)
    $uninstaller=Join-Path $target 'unins000.exe'
    $installedExe=Join-Path $target 'AXEVoice.WinUI3.exe'
    $activeCount=0
    do {
        $active=@(Get-CimInstance Win32_Process -Filter "Name LIKE 'AXE-Voice-WinUI3-Native-Lab-Setup%'")
        $activeCount=$active.Count
        if((Test-Path -LiteralPath $uninstaller -PathType Leaf) -and
           (Test-Path -LiteralPath $installedExe -PathType Leaf) -and
           $activeCount -eq 0) {
            return
        }
        Start-Sleep -Milliseconds 400
    } while([DateTime]::UtcNow -lt $deadline)
    $default=Join-Path $env:LOCALAPPDATA 'Programs/AXEVoiceWinUI3NativeLab/unins000.exe'
    $wrongLocation=Test-Path -LiteralPath $default -PathType Leaf
    $actualExe=Test-Path -LiteralPath $installedExe -PathType Leaf
    $actualUninstaller=Test-Path -LiteralPath $uninstaller -PathType Leaf
    $setupLogExists=Test-Path -LiteralPath $setupLog -PathType Leaf
    $traceSize=if($setupLogExists){(Get-Item -LiteralPath $setupLog).Length}else{0}
    # Do not dump raw Inno logs into public PR CI: they may contain user
    # filesystem paths or other environment data. Emit existence and sizes.
    throw "WinUI 3 lab installation incomplete: exe=$actualExe, uninstaller=$actualUninstaller, activeSetup=$activeCount, defaultDirectoryHasUninstaller=$wrongLocation, setupLogExists=$setupLogExists, setupLogBytes=$traceSize. No process was killed."
}
function Verify-Payload {
    foreach($file in @(Get-ChildItem -LiteralPath $source -Recurse -File)) {
        $relative=$file.FullName.Substring($source.Length+1)
        $installed=Join-Path $target $relative
        Require-File $installed
        if((Hash $file.FullName) -ne (Hash $installed)) {
            throw "Installed WinUI 3 lab file hash mismatch: $relative"
        }
    }
}
function Invoke-Uninstall {
    $uninstaller=Join-Path $target 'unins000.exe'
    Require-File $uninstaller
    Wait-ProcessBounded $uninstaller @(
        '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
    ) 'WinUI 3 lab uninstall'
    # The Inno uninstaller can also spawn a helper which outlives the initial
    # bootstrap handle. Require the real installed payload to be gone before
    # asserting success. Never delete any files or terminate unknown processes.
    $deadline=[DateTime]::UtcNow.AddSeconds(120)
    $remaining=@()
    do {
        $remaining=@(Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object {
            $relative=$_.FullName.Substring($source.Length+1)
            Test-Path -LiteralPath (Join-Path $target $relative)
        })
        $active=@(Get-CimInstance Win32_Process -Filter "Name LIKE 'unins000%'")
        if($remaining.Count -eq 0 -and $active.Count -eq 0 -and
           !(Test-Path -LiteralPath $link)) { return }
        Start-Sleep -Milliseconds 400
    } while([DateTime]::UtcNow -lt $deadline)
    throw "WinUI 3 lab uninstall incomplete: remainingPayload=$($remaining.Count), activeUninstaller=$($active.Count), desktopShortcutExists=$(Test-Path -LiteralPath $link)"

}
$common=@('/SP-','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',
          "/DIR=`"$target`"","/LOG=`"$setupLog`"")
try {
    Invoke-Setup $common
    Wait-InstalledCompletion
    Verify-Payload
    if(Test-Path -LiteralPath $link){throw 'Default installation unexpectedly created a desktop shortcut'}
    Invoke-Uninstall
    # Explicit user opt-in should create the correct executable link and
    # remove it on uninstall, without launching or connecting the softphone.
    Invoke-Setup ($common + @('/TASKS=desktopicon'))
    Wait-InstalledCompletion
    Verify-Payload
    Require-File $link
    $shell=New-Object -ComObject WScript.Shell
    $shortcut=$shell.CreateShortcut($link)
    try {
        if(![string]::Equals([IO.Path]::GetFullPath($shortcut.TargetPath),
             [IO.Path]::GetFullPath((Join-Path $target 'AXEVoice.WinUI3.exe')),
             [StringComparison]::OrdinalIgnoreCase)) {
            throw 'WinUI 3 desktop shortcut target differs from the installed app'
        }
    } finally {
        if($shortcut){[void][Runtime.InteropServices.Marshal]::ReleaseComObject($shortcut)}
        if($shell){[void][Runtime.InteropServices.Marshal]::ReleaseComObject($shell)}
    }
    Invoke-Uninstall
} finally {
    # Never kill unknown processes, delete user files, or remove a shortcut
    # whose identity was not proved. Failure remains visible to CI.
}
Write-Host "WinUI3 native LAB installer PASS: default off, opt-in shortcut, content hashes, uninstall, version $version."
Get-FileHash -LiteralPath $installer -Algorithm SHA256 | Format-List
