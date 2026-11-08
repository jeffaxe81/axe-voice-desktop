# Static XAML contract for the WinUI 3 light/dark palette.
# The runtime theme and final appearance still require screenshot testing.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$xamlPath = Join-Path $root 'apps/winui3/MainWindow.xaml'
$codePath = Join-Path $root 'apps/winui3/MainWindow.xaml.cs'
$source = Get-Content -LiteralPath $xamlPath -Raw -Encoding UTF8
$code = Get-Content -LiteralPath $codePath -Raw -Encoding UTF8
[xml]$xaml = $source # Fail on malformed XML or invalid attribute syntax.
$ns = New-Object System.Xml.XmlNamespaceManager($xaml.NameTable)
$ns.AddNamespace('ui', 'http://schemas.microsoft.com/winfx/2006/xaml/presentation')
$ns.AddNamespace('x', 'http://schemas.microsoft.com/winfx/2006/xaml')
$xamlNs = 'http://schemas.microsoft.com/winfx/2006/xaml'
$dictionaries = $xaml.SelectNodes('//ui:ResourceDictionary.ThemeDictionaries/ui:ResourceDictionary', $ns)
$palettes = @{}
foreach ($dict in $dictionaries) {
    $variant = $dict.GetAttribute('Key', $xamlNs)
    if (!$variant -or $palettes.ContainsKey($variant)) { throw 'Duplicate or missing theme key.' }
    $colors = @{}
    foreach ($entry in $dict.SelectNodes('ui:SolidColorBrush', $ns)) {
        $key = $entry.GetAttribute('Key', $xamlNs)
        $value = $entry.GetAttribute('Color')
        if (!$key -or $colors.ContainsKey($key) -or !$value) { throw "Invalid brush in $variant" }
        $colors[$key] = $value
    }
    $palettes[$variant] = $colors
}
foreach ($variant in @('Light','Dark','Default')) {
    if (!$palettes.ContainsKey($variant)) { throw "Missing $variant theme resources." }
}
$references = [regex]::Matches($source, '\{ThemeResource (Axv[A-Za-z]+Brush)\}') |
    ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
if ($references.Count -lt 12) { throw 'Not all custom surfaces use theme resources.' }
foreach ($variant in @('Light','Dark','Default')) {
    foreach ($token in $references) {
        if (!$palettes[$variant].ContainsKey($token)) {
            throw "Unresolved $token in $variant theme."
        }
    }
}
$required = @('AxvShellBrush','AxvTitleBrush','AxvStatusBackgroundBrush',
              'AxvInputBrush','AxvKeyBackgroundBrush','AxvKeyTextBrush',
              'AxvDetailsBackgroundBrush','AxvDetailsTextBrush')
foreach ($token in $required) {
    if ($palettes['Light'][$token] -eq $palettes['Dark'][$token]) {
        throw "The $token palette does not change between light/dark themes."
    }
}
if ($source -match '(?:Background|Foreground)\s*=\s*"(?:#[0-9a-fA-F]{3,8}|White)"') {
    throw 'Found hard-coded light theme brush on a user-facing control.'
}
if ($code -notmatch 'MainShell.RequestedTheme\s*=\s*ElementTheme.Light' -or
    $code -notmatch 'MainShell.RequestedTheme\s*=\s*_dark\s*\?\s*ElementTheme.Dark\s*:\s*ElementTheme.Light' -or
    $code -notmatch 'AutomationProperties.SetName\(ThemeToggle') {
    throw 'Theme toggle has no stateful theme/accessible label contract.'
}
# Readable compact UI must retain at least WCAG AA 4.5:1 text contrast.
# This statically checks palette pairs; screenshots and accessibility audits
# on a real Windows monitor are still required.
function Get-LinearLuminance([string]$Raw) {
    $hex=if($Raw -eq 'White') {'#FFFFFF'} else {$Raw}
    if($hex -notmatch '^#[0-9a-fA-F]{6}$') {
        throw "Unrecognized theme color syntax"
    }
    $channels=@()
    foreach($offset in @(1,3,5)) {
        $channel=[Convert]::ToInt32($hex.Substring($offset,2),16)/255.0
        if($channel -le 0.04045) {
            $channels+=($channel/12.92)
        } else {
            $channels+=[Math]::Pow(($channel+0.055)/1.055,2.4)
        }
    }
    return 0.2126*$channels[0]+0.7152*$channels[1]+0.0722*$channels[2]
}
function Get-Contrast([string]$Foreground,[string]$Background) {
    $lightness=Get-LinearLuminance $Foreground
    $darkness=Get-LinearLuminance $Background
    if($darkness -gt $lightness) {
        $temp=$lightness;$lightness=$darkness;$darkness=$temp
    }
    return ($lightness+0.05)/($darkness+0.05)
}
$contrastPairs=@(
    @('AxvTitleBrush','AxvShellBrush'),
    @('AxvSecondaryBrush','AxvShellBrush'),
    @('AxvStatusTextBrush','AxvStatusBackgroundBrush'),
    @('AxvStatusDetailBrush','AxvStatusBackgroundBrush'),
    @('AxvInputTextBrush','AxvInputBrush'),
    @('AxvKeyTextBrush','AxvKeyBackgroundBrush'),
    @('AxvDetailsTitleBrush','AxvDetailsBackgroundBrush'),
    @('AxvDetailsTextBrush','AxvDetailsBackgroundBrush')
)
foreach($variant in @('Light','Dark','Default')) {
    foreach($pair in $contrastPairs) {
        $foreground=$palettes[$variant][$pair[0]]
        $background=$palettes[$variant][$pair[1]]
        $ratio=Get-Contrast $foreground $background
        if($ratio -lt 4.5) {
            throw "Insufficient theme contrast: $variant/$($pair[0])/$($pair[1]) = $([Math]::Round($ratio,2)):1"
        }
    }
}
Write-Host "WinUI3 contrast contract: 8 text pairs x 3 themes satisfy 4.5:1 (static palette)."
Write-Host "WinUI3 theme contract: $($references.Count) references, $($palettes.Count) palettes, PASS."
