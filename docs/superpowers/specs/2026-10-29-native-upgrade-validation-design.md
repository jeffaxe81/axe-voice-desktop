# Native upgrade validation and process lifetime

Continue six requested specialist fronts from0.68. Close interactive postinstall and real setup contention coverage gaps using production Inno without /SILENT for interactive flow. All GUI actions scoped to spawned process IDs; bounded waits and graceful closure; no PBX, physical input or forced app termination.

A detached audio inventory probe can outlive its owner. The application Running marker must remain until Windows process termination, not only main.cpp owner destructors. Keep normal NativeInstallGuard scope RAII for reusable/test guards, add explicit process lifetime option used by the native GUI. Destructor retains the process-owned marker; OS closes it on exit. Explicit release remains for scoped use, never called for the GUI marker. Test real guard/AudioInventoryAsync composition in a child with blocked detached probe, marker visible after owner destruction, and marker absent after process exit. No production test hooks.

Alternatives considered: joining inventory probes can hang GUI shutdown on blocked drivers; RAII global declaration ordering alone cannot cover detached threads. Process lifetime avoids both. Choice by implementer under continuing development authorization.

Candidate0.68.1. Preserve multiple app holders, reciprocal admission and fail closed errors. Legacy startup race, interactive physical accessibility, hardware/Digitro, signing, historical obsolete-file cleanup remain pending. New tests do not establish active-call/hardware acceptance.
