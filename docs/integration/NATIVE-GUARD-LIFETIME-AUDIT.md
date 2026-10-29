# Native guard lifetime audit

Reviewed 2026-10-29 using public repository source only. This is a source audit; Windows native GUI teardown and installer overlap runtime coverage remain pending.

## Finding: marker lifetime does not cover detached audio probes

Severity: medium for the guard's stated lifetime guarantee; practical installer impact is unverified.

`src/main.cpp:70` declares `g_native_install_guard` before the audio owners and engine selection (`:163`, `:167`, `:183`). Normal ordered destruction of these objects in this translation unit therefore closes the Running marker after their destructors. However, `AudioInventoryAsync::start_pending()` detaches the probe thread (`src/audio_inventory_async.cpp:50-69`), and `close()` only invalidates its shared state (`:88-93`). Its destructor calls `close()` without joining (`:28`). Both real GUI inventory workers use `inspect_windows_audio_devices`.

A probe blocked in the Windows endpoint APIs can consequently outlive both inventory owner destructors and the guard destructor. The shared State preserves the probe's memory safety, but does not preserve the marker. A setup process can observe no Running marker during the remaining process shutdown interval even though a detached probe has not completed. This establishes a gap in the claim that the marker outlives all audio work. It does not establish that Windows permits replacement of the still-mapped executable or DLLs, or that a shipped installer actually overwrites them in that interval.

`tests/test_audio_inventory_shutdown.cpp` already exercises owner destruction while a gated probe remains blocked. It demonstrates the deliberate detached lifetime, not native application exit or installer safety. No production change or source-pattern test was added for this audit.

## Checked lifetime boundaries

- The marker is acquired at the beginning of `wWinMain` (`src/main.cpp:1804-1819`), before window creation, endpoint watcher startup, or user-triggered native runtime startup. Registration/window creation failure returns retain the global marker through subsequent owner destruction.
- Pre-entry global initialization allocates audio State, watcher Impl, and native engine Impl. The inspected constructors do not start COM enumeration, rendering, native SIP initialization, or threads. `BaresipNativeLibreReactor` construction stores hooks; `start()` creates its worker. Thus no concrete unguarded pre-entry SIP/audio activity was found in application constructors. This does not audit Windows loader activity or external SDK DLL initialization.
- Native engine destruction calls `stop()`; reactor stop joins its worker from the GUI thread. Tone preview destruction requests cancellation and joins. Incoming ringtone shutdown also joins. Their main.cpp owners are declared after the guard. These are stronger lifetime boundaries than the detached inventory workers.
- The inspected cross-translation-unit native adapter/engine process-owner statics are atomic pointers, not independent worker owners requiring teardown. No additional application namespace-scope SIP/audio worker owner was identified in the inspected native engine, reactor, adapter, watcher, tone, ringtone, and inventory sources. External SDK/library global destructors are outside this conclusion; declaration order in main.cpp cannot establish their ordering.
- `NativeInstallGuard::acquire()` performs no throwing C++ allocations after creating the candidate handle. Every refusal after candidate creation closes it; successful acquisition transfers it to the member. The SID/name builder catches allocation exceptions and uses local RAII for token and SID storage. No concrete exception-induced handle leak was found in these paths. The detail Handle/SidText wrappers are implicitly copyable, but their current local uses never copy them.

## Measurable follow-up

Add a separate Windows test executable composing the real `NativeInstallGuard` and `AudioInventoryAsync` public interfaces with a gated probe and unique test object names. Destroy the inventory owner and then the guard while the probe is gated; another process should record marker absence and probe liveness. This reproduces the lifetime gap without a production hook or source regex. A corrected lifetime policy should keep the marker present until that probe finishes, or explicitly retain the marker for the process lifetime and rely on OS handle cleanup.

For the shipped GUI, use Windows debugger/ETW evidence spanning normal shutdown and a real endpoint enumeration in progress. Record actual destructor/worker completion order, last marker-handle closure, process termination, and installer admission. A marker observation while the GUI is still running, or immediately before returning from its message loop, does not prove global teardown coverage. Until such evidence exists, report native process teardown and installer overlap acceptance as pending.
