# Pure identity predicates used by the real Win32 postinstall fixture. Paths are
# not ownership: lineage and creation times are required before any GUI action.
function Test-NativePostinstallChildIdentity {
    param([uint32]$ChildId,[uint32]$ParentId,[DateTime]$StartedAt,$Parent)
    $StartedAt = $StartedAt.ToUniversalTime()
    return $ChildId -ne $ParentId -and $ParentId -eq $Parent.Id -and
        $StartedAt -ge $Parent.StartedAt -and
        ($null -eq $Parent.ExitedAt -or $StartedAt -le $Parent.ExitedAt)
}
function Test-NativePostinstallSameIdentity {
    param([DateTime]$Expected,[DateTime]$Actual)
    # CIM reports creation timestamps to microseconds; ignore only its missing
    # 100 ns digit, not a millisecond window which could admit PID reuse.
    $ExpectedTicks = $Expected.ToUniversalTime().Ticks
    $ActualTicks = $Actual.ToUniversalTime().Ticks
    return ($ExpectedTicks - ($ExpectedTicks % 10)) -eq ($ActualTicks - ($ActualTicks % 10))
}
