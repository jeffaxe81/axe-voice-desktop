$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'native_postinstall_identity.ps1')
$Start = [DateTime]::new(2026,10,29,12,0,0,[DateTimeKind]::Utc)
$Parent = [pscustomobject]@{ Id=40; StartedAt=$Start; ExitedAt=$Start.AddSeconds(10) }
if (!(Test-NativePostinstallChildIdentity 41 40 $Start.AddSeconds(2) $Parent)) { throw 'Actual child inside parent lifetime must be accepted' }
if (Test-NativePostinstallChildIdentity 42 99 $Start.AddSeconds(2) $Parent) { throw 'Unrelated same-path process must not be accepted' }
if (Test-NativePostinstallChildIdentity 43 40 $Start.AddSeconds(11) $Parent) { throw 'Child of recycled parent PID must not be accepted' }
if (Test-NativePostinstallChildIdentity 40 40 $Start.AddSeconds(2) $Parent) { throw 'Self-parent identity must not be accepted' }
if (Test-NativePostinstallChildIdentity 44 40 $Start.AddSeconds(-1) $Parent) { throw 'Child older than its parent must not be accepted' }
if (!(Test-NativePostinstallSameIdentity $Start $Start)) { throw 'Identical creation times must match' }
if (Test-NativePostinstallSameIdentity $Start $Start.AddMilliseconds(1)) { throw 'Recycled PID with changed creation time must not match' }
Write-Output 'Postinstall lineage rejects unrelated same-path processes and recycled process identities'
