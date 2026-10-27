# Candidate0.59 — asynchronous device diagnostics

Implement the requirements recorded in public historical PR30/audio/settings documentation, without importing private implementations. Ten developer fronts split worker, retry, selection, UI, help, shutdown, epochs, channel isolation, launch failures and independent review. The environment supports six agents beside the coordinator, so execute in two waves.

Use two independent AudioInventoryAsync instances for Audio and Settings. One probe per instance; repeated requests advance generation and coalesce one pending scan. Data-only detached shared state can outlive a blocked Core Audio probe, so cancel/close discard pending/results without waiting for the driver. Probe owns dependencies, never HWND or SIP objects. Exceptions become inventory errors; failed thread creation preserves pending with1000ms retry pacing. UI timer alone applies current data on the matching visible page; dropdowns must be closed before Audio launch/application. Leaving/compact navigation cancels the visual generation.

Error preserves selections and prior lists but disables controls/preview until fresh success. Native selections retain explicit/default semantics without silent fallback. Existing engine owner preflight remains fresh and routes fixed per session; no live route migration. Settings gets its independent native diagnostic button. Smoke launches no probes and touches no physical hardware, SIP or clipboard.

Goals are responsiveness and stale-result exclusion, not proof of audio. Physical hotplug, blocked driver behavior and accessibility still need Windows hardware acceptance, together with deferred Dígitro/headset/IVR tests. Package0.59.0 after full Windows/native/installer gates.
