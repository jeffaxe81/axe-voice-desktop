AXE Voice Native Candidate 0.61.0

This is a validation build, not a production-approved release.
Enter server, extension, port and session password in Settings, then Register.
Passwords are not saved to account/config files. Stop the engine before changing accounts.
TLS requires a trusted CA PEM file. Plain UDP is unencrypted; select it only according to your PBX configuration.
SIP TLS does not encrypt RTP. This candidate does not claim SRTP support.
Select microphone and voice output in the Audio panel before registration. Those WASAPI endpoint IDs are fixed for the session; stop/re-register to change them.
Without explicit selections, Windows default communications capture/playback devices are used.
Missing selected endpoints block new calls/answering; reconnect or stop and choose another endpoint. Incoming ringtone uses an independent output; its empty selection follows the Windows multimedia default. Missing explicit ringtone outputs do not fall back. Answer/decline silences the current alert, including when the SIP action fails; the visual call remains available.
Physical headset audio, bidirectional RTP and actual Digitro PBX calls require acceptance testing.
Third-party license texts are included under licenses/.
The installer and executable are unsigned. File hashes identify artifacts, not publisher certification.

During an established native call, MUTE changes the SDK microphone flag; HOLD/RESUME requests local hold state. A local hold flag is not confirmation of remote SIP negotiation. Validate microphone silence and peer hold/resume behavior on the target PBX/headset. For direct transfer, enter the destination during an established call, click TRANSFERIR and review it before sending. One request per call; SDK submission is not confirmed central acceptance. Attended transfer remains unavailable.

F2 readiness separates current dial-button availability from physical audio acceptance. An enabled button does not certify working media; the engine revalidates each command.

On Audio, COPIAR STATUS or Ctrl+Shift+D copies only local boolean readiness indicators as Unicode text. No account, destination, password or device ID is included. No network or file export occurs. Clipboard contents remain until replaced; Windows history/sync follows your system settings. Physical PBX/headset acceptance remains pending.

During an established call, enable DTMF ON (SIP INFO) and use the on-screen keypad for0-9, * and #. The profile uses SIP INFO only (250 ms per SDK request); RTP-event DTMF and automatic fallback are not provided. Hold/transfer or call replacement clears DTMF mode. A busy command queue rejects the press visibly. Submission does not confirm remote receipt; actual PBX/IVR behavior remains pending. No digit history is stored.

The native duration label measures local time from the first verified established-call observation, including hold while the call remains InCall. Ringing/outgoing time is excluded. Call replacement, stop or lost proof resets the timer. Elapsed time does not certify audio or remote acceptance.

Audio and Settings device queries now run asynchronously in independent channels. Repeated requests coalesce; leaving a pane discards its visual update. Cancel/close does not interrupt a blocked driver. Errors retain previous selections while disabling controls until fresh success; native call routes remain fixed for the session.

Audio device notifications now schedule debounced inventory updates only on the visible Audio pane with closed dropdowns. A periodic/manual fallback remains if the observer is unavailable. Call routes do not change automatically; physical hotplug and target PBX acceptance remain pending.
