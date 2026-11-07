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
Write-Host "WinUI3 theme contract: $($references.Count) references, $($palettes.Count) palettes, PASS."
