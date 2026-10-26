# Candidate0.58 — local call duration

Continue with five developer fronts while physical Dígitro/headset acceptance stays pending. Add a native-only duration label between the runtime banner and page content, visible also in MINI. Measure monotonic local time from the first verified established observation; include hold while SIP remains InCall. Reset on call/session replacement, invalid proof, stop or non-established state. Never count ringing/outgoing as established duration, and never infer media or remote acceptance from elapsed time.

Use a pure observation-driven clock, a separate HH:MM:SS formatter and the existing UI100ms polling with GetTickCount64. Clamp backward samples; support zero initial uptime and uint64 extremes without overflow. Format hours without wrapping at99. No call history, account data, persistence or network side effect. The diagnostic exporter retains boolean-only input.

Separate supporting fronts: a source metadata gate rejects divergent/missing/ambiguous versions before Windows build; a real local TLS regression separates the bounded2s handshake timeout from0.2s read polling. Preserve CA/hostname checks, strict negative-peer assertions and application registration deadlines. These changes address observed CI handshake aborts without relaxing SIP/TLS acceptance.

Tests cover pure policy/format, silent native UI label/layout, source-version mismatch fixtures and delayed real TLS completion. Physical Windows accessibility and target PBX calls remain pending. Product0.58.0.
