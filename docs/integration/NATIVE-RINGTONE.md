# Native candidate0.55 — incoming ringtone

Verified native incoming calls trigger a repeated local 250 ms tone on the selected ringtone output. Empty selection follows the Windows multimedia default; microphone and conversation playback retain their separate session routes. An explicit missing output never falls back. Rendering failures retain the visual call and display a warning.

A single worker serializes tones. Session/call/output changes cancel the old generation; obsolete results cannot replace current state. Answer/decline intent silences that call even if the command fails. A new call may ring. Answer and hangup validate the displayed call identity against current native state. Manual preview is disabled during a verified incoming call; unattended UI smoke never renders sound.

Cancellation is cooperative before audio start, during buffer delivery and during drain. A driver API may block; no hard acoustic stop deadline is claimed. Shutdown cancels and joins the worker. Tests cover policy, replacement, errors/retry, serialization, cancellation and silent UI selection. The native loopback fixtures cover signaling registration, not incoming acoustic delivery.

Physical Windows acceptance remains required: incoming calls on the target Dígitro PBX, audibility on explicit/default outputs, output loss/reconnection, and stop latency on answer/decline. No production approval or actual PBX compatibility is inferred from automated tests.
