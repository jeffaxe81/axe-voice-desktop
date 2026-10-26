# Native candidate0.57 — DTMF via SIP INFO

During a verified established native call, enable **DTMF ON (SIP INFO)** below the call controls. The on-screen keypad sends0–9, * and # instead of editing the destination. Turn the mode off to return to editing; typing in the destination field always edits it for dialing/transfer. Mode is bound to the displayed session/call and clears on replacement, local hold, attempted transfer or stop. The SDK-free build does not expose this control.

The profile explicitly sets dtmfmode=info. The SDK port rejects other account modes; there is no automatic RTP fallback or local tone generation. Pinned Baresip4.12 sends application/dtmf-relay with Duration=250 per accepted digit. Only one UI command slot is available: a full slot rejects the press visibly. Repeated accepted identical digits are intentional and allowed. No digit history or digit-bearing application diagnostics are kept.

Both UI and owner dispatch validate identity/proof/state. The port independently verifies the owner thread, selected registered UA, established call, hold/transfer state, allowed key and account mode. A send error preserves the call and reports a fixed diagnostic. SDK submission is not asynchronous INFO acceptance or proof of IVR receipt.

Portable tests cover policy/mode, stale identity and command repetition. SDK doubles cover wrong thread/token, mode, hold/transfer, errors and reentrant closure. Real pinned SDK parsing validates the account mode; native builds resolve the actual DTMF API, and existing signaling fixtures/installer gates remain required. Actual in-dialog INFO delivery, remote rejection, timing and navigation of the Dígitro IVR remain pending, together with physical audio/headset acceptance. This is still a release candidate.
