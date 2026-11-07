<#
 Capture the actual onscreen AXE Voice window for human acceptance.
 This does not upload, redact, or compare screenshots automatically.
 Never run on an account screen containing a real SIP secret.
 Examples:
   ./scripts/capture-windows-design.ps1 -Edition Win32 -Scenario 'dialer-light' -AllowLocalCapture
   ./scripts/capture-windows-design.ps1 -Edition WinUI3 -Scenario 'dialer-dark' -AllowLocalCapture
#>
[CmdletBinding()]
param(
    [ValidateSet('Win32','WinUI3')][string]$Edition='Win32',
    [ValidatePattern('^[a-zA-Z0-9_-]{1,48}$')][string]$Scenario='dialer',
    [string]$OutputDirectory=(Join-Path $env:LOCALAPPDATA 'AXEVoice\design-evidence'),
    [switch]$AllowLocalCapture
)
$ErrorActionPreference='Stop'
if(!$AllowLocalCapture){
    throw 'Explicit -AllowLocalCapture required; images can contain phone numbers or other personal data.'
}
if(![Environment]::UserInteractive){throw 'Use an interactive Windows desktop to capture the actual UI.'}
$processName=if($Edition -eq 'Win32'){'AXE-Voice'}else{'AXEVoice.WinUI3'}
$instances=@(Get-Process -Name $processName -ErrorAction SilentlyContinue | Where-Object {
    $_.MainWindowHandle -ne [IntPtr]::Zero
})
if($instances.Count -ne 1){
    throw "Expected exactly one visible $Edition window, found $($instances.Count). Open only one AXE Voice instance."
}
$process=$instances[0]
Add-Type -AssemblyName System.Drawing
if(-not ('AxvDesignNativeWindow' -as [type])){
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AxvDesignNativeWindow {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {public int Left,Top,Right,Bottom;}
    [DllImport("user32.dll", SetLastError=true)]
    [return:MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetWindowRect(IntPtr hwnd,out RECT bounds);
    [DllImport("user32.dll")]
    [return:MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsIconic(IntPtr hwnd);
    [DllImport("user32.dll")]
    [return:MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hwnd);
}
'@
}
$handle=$process.MainWindowHandle
if([AxvDesignNativeWindow]::IsIconic($handle)){throw 'Restore the app from minimized state before capture.'}
if(-not [AxvDesignNativeWindow]::SetForegroundWindow($handle)){
    throw 'Could not make AXE Voice the foreground window. Bring it to the front and retry.'
}
Start-Sleep -Milliseconds 600
$bounds=[AxvDesignNativeWindow+RECT]::new()
if(-not [AxvDesignNativeWindow]::GetWindowRect($handle,[ref]$bounds)){
    throw 'GetWindowRect failed for the AXE Voice window.'
}
$w=$bounds.Right-$bounds.Left
$h=$bounds.Bottom-$bounds.Top
if($w -lt 160 -or $h -lt 200 -or $w -gt 2200 -or $h -gt 3200){
    throw "Unexpected AXE Voice screenshot size: ${w}x${h}."
}
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$filename='AXE-Voice-{0}-{1}-{2:yyyyMMdd_HHmmss}.png' -f $Edition,$Scenario,[DateTime]::Now
$destination=Join-Path $OutputDirectory $filename
$bitmap=[System.Drawing.Bitmap]::new($w,$h)
try {
    $graphics=[System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen([System.Drawing.Point]::new($bounds.Left,$bounds.Top),
                                 [System.Drawing.Point]::Empty,
                                 [System.Drawing.Size]::new($w,$h),
                                 [System.Drawing.CopyPixelOperation]::SourceCopy)
    } finally {$graphics.Dispose()}
    $bitmap.Save($destination,[System.Drawing.Imaging.ImageFormat]::Png)
} finally {$bitmap.Dispose();$process.Dispose()}
Write-Host "Captured local AXE Voice UI evidence: $destination ($w x $h px)"
Write-Warning 'Review and redact phone numbers/identifiers before sharing. No automatic upload was performed.'
