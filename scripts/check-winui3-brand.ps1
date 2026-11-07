# WinUI 3 branding asset contract; visual fidelity still needs a real screenshot.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$asset=Join-Path $root 'assets/axe-voice.ico'
$xaml=Get-Content -LiteralPath (Join-Path $root 'apps/winui3/MainWindow.xaml') -Raw -Encoding UTF8
$csproj=Get-Content -LiteralPath (Join-Path $root 'apps/winui3/AXEVoice.WinUI3.csproj') -Raw -Encoding UTF8
$code=Get-Content -LiteralPath (Join-Path $root 'apps/winui3/MainWindow.xaml.cs') -Raw -Encoding UTF8
[xml]$xml=$xaml
[xml]$project=$csproj
$ns=[System.Xml.XmlNamespaceManager]::new($xml.NameTable)
$ns.AddNamespace('ui','http://schemas.microsoft.com/winfx/2006/xaml/presentation')
$brand=$xml.SelectSingleNode('//ui:Image[@Source="ms-appx:///Assets/axe-voice.ico"]',$ns)
if(!$brand -or $brand.GetAttribute('Height') -ne '32' -or $brand.GetAttribute('Width') -ne '32'){
    throw 'WinUI3 title bar does not use the compact approved brand.'
}
if($xaml -match '<TextBlock\s+Text="A"'){throw 'Generic placeholder A still present in title bar.'}
if($code -notmatch 'AppWindow\.SetIcon\('){throw 'WinUI3 window title icon not bound.'}
$entry=$project.Project.ItemGroup.Content|Where-Object { $_.Link -eq 'Assets\axe-voice.ico' }
if(!$entry -or $entry.CopyToPublishDirectory -ne 'Always'){
    throw 'Brand ICO is not linked to the publish output.'
}
if(!(Test-Path -LiteralPath $asset -PathType Leaf)){throw 'Approved ICO is missing.'}
$bytes=[System.IO.File]::ReadAllBytes($asset)
if($bytes.Length -lt 100 -or $bytes[0] -ne 0 -or $bytes[1] -ne 0 -or
   $bytes[2] -ne 1 -or $bytes[3] -ne 0) { throw 'Invalid Windows ICO header.' }
$images=[int]$bytes[4]+([int]$bytes[5] -shl 8)
if($images -lt 3){throw 'Brand ICO must have multiple DPI variants.'}
Write-Host "WinUI3 icon contract PASS: $images variants, $($bytes.Length) bytes, approved asset."
