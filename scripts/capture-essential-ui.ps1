#requires -Version 5.1
<#
.SYNOPSIS
  Captures the actual AXE Voice Win32 window for visual review of Option 1.
.DESCRIPTION
  Launches the native application without SIP credentials, captures its window
  with PrintWindow, and writes a PNG. This is visual evidence, NOT SIP proof.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference='Stop'
if(-not (Test-Path -LiteralPath $Executable -PathType Leaf)){
    throw "AXEVoice executable missing at $Executable"
}
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AxvWindowShot {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left,Top,Right,Bottom; }
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool GetWindowRect(IntPtr hwnd,out RECT rect);
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PrintWindow(IntPtr hwnd,IntPtr hdc,uint flags);
    [DllImport("user32.dll",SetLastError=true)]
    public static extern IntPtr GetDlgItem(IntPtr hwnd,int controlId);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hwnd);
}
'@
$process=$null
$bitmap=$null
$graphics=$null
try {
    $process=Start-Process -FilePath (Resolve-Path -LiteralPath $Executable).Path -PassThru
    $handle=[IntPtr]::Zero
    for($attempt=0;$attempt -lt 50;$attempt++){
        Start-Sleep -Milliseconds 120
        $process.Refresh()
        if($process.HasExited){throw "AXE Voice exited before visual capture"}
        $handle=$process.MainWindowHandle
        if($handle -ne [IntPtr]::Zero){break}
    }
    if($handle -eq [IntPtr]::Zero){throw "AXE Voice main window unavailable"}
    Start-Sleep -Milliseconds 350
    $bounds=New-Object AxvWindowShot+RECT
    if(-not [AxvWindowShot]::GetWindowRect($handle,[ref]$bounds)){
        throw "Unable to measure AXE Voice window"
    }
    $width=$bounds.Right-$bounds.Left
    $height=$bounds.Bottom-$bounds.Top
    if($width -lt 340 -or $height -lt 600){
        throw "Unexpected option-1 window dimensions: $width x $height"
    }
    $bitmap=[System.Drawing.Bitmap]::new($width,$height)
    $graphics=[System.Drawing.Graphics]::FromImage($bitmap)
    $dc=$graphics.GetHdc()
    $printed=$false
    try {
        $printed=[AxvWindowShot]::PrintWindow($handle,$dc,2)
        if(-not $printed){$printed=[AxvWindowShot]::PrintWindow($handle,$dc,0)}
    } finally {
        $graphics.ReleaseHdc($dc)
    }
    if(-not $printed){throw "Win32 PrintWindow failed; screenshot not trustworthy"}
    $center=$bitmap.GetPixel([Math]::Min($width-1,[int]($width/2)),
                             [Math]::Min($height-1,[int]($height/2)))
    $header=$bitmap.GetPixel([Math]::Min($width-1,10),[Math]::Min($height-1,90))
    if(($center.ToArgb() -eq 0) -and ($header.ToArgb() -eq 0)){
        throw "Screenshot appears blank; refusing to publish"
    }
    $folder=Split-Path -Parent $Output
    if($folder -and -not (Test-Path -LiteralPath $folder)){
        New-Item -ItemType Directory -Force -Path $folder | Out-Null
    }
    # Inspect real child HWNDs, not just an idealized coordinate model.
    $required=@(1001,1002,2001,2013,2011,2015,2301,2302,2303,2800)
    $required+=@(2100..2111)
    foreach($id in $required){
        $control=[AxvWindowShot]::GetDlgItem($handle,[int]$id)
        if($control -eq [IntPtr]::Zero){
            throw "Approved Option 1 control missing from real window: $id"
        }
        if(-not [AxvWindowShot]::IsWindowVisible($control)){
            throw "Approved Option 1 control is hidden: $id"
        }
    }
    for($digit=0;$digit -lt 12;$digit++){
        $key=[AxvWindowShot]::GetDlgItem($handle,2100+$digit)
        $r=New-Object AxvWindowShot+RECT
        if(-not [AxvWindowShot]::GetWindowRect($key,[ref]$r)){
            throw "Unable to inspect digit $digit"
        }
        if($r.Right -le $r.Left -or $r.Bottom -le $r.Top){
            throw "Digit has invalid bounds: $digit"
        }
        if(($digit % 3) -ne 2){
            $next=[AxvWindowShot]::GetDlgItem($handle,2101+$digit)
            $n=New-Object AxvWindowShot+RECT
            [void][AxvWindowShot]::GetWindowRect($next,[ref]$n)
            if($r.Right -gt $n.Left){throw "Horizontal keypad overlap at digit $digit"}
        }
        if($digit -lt 9){
            $next=[AxvWindowShot]::GetDlgItem($handle,2103+$digit)
            $n=New-Object AxvWindowShot+RECT
            [void][AxvWindowShot]::GetWindowRect($next,[ref]$n)
            if($r.Bottom -gt $n.Top){throw "Vertical keypad overlap at digit $digit"}
        }
    }
    $bitmap.Save($Output,[System.Drawing.Imaging.ImageFormat]::Png)
    Write-Host ("AXE Voice visual evidence: {0} ({1} x {2})" -f $Output,$width,$height)
} finally {
    if($graphics){$graphics.Dispose()}
    if($bitmap){$bitmap.Dispose()}
    if($process){
        try {
            $process.Refresh()
            if(-not $process.HasExited){Stop-Process -Id $process.Id -Force}
        } catch { Write-Warning "Unable to stop visual probe: $_" }
        $process.Dispose()
    }
}
