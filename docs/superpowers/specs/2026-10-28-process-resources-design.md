# Partial Windows process startup resources — candidate0.65.1

Continue the authorized security work with four independent testing audits. Keep C++17/Win32, the existing public worktree, fixed executable and argument list, explicit inherited-handle list and suspended/job-controlled launch.

The concrete hypothesis is that allocation failure after CreatePipe/CreateFile leaks local handles because only job/process/stdin members have cleanup. Reproduce on Windows using allocation injection and GetProcessHandleCount before modifying production.

Approaches: moving allocations before resource acquisition reduces exposure but leaves future edits fragile; scoped ownership covers every exit including exceptions. Choose scoped ownership plus a public boolean failure boundary, leaving behavior and launch policy intact. No new dependencies. Failed allocation must return false, ERROR_NOT_ENOUGH_MEMORY and Failed state; local handles and initialized attributes must close, and retry must remain possible.

Scope excludes target PBX/headset acceptance, signing and SRTP. Audits with no finding are assurance, not new features. Same-candidate reinstall does not establish arbitrary historical upgrades.
