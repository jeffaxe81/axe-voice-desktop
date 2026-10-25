AXE Voice Native Candidate 0.54.0

This is a validation build, not a production-approved release.
Enter server, extension, port and session password in Settings, then Register.
Passwords are not saved to account/config files. Stop the engine before changing accounts.
TLS requires a trusted CA PEM file. Plain UDP is unencrypted; select it only according to your PBX configuration.
SIP TLS does not encrypt RTP. This candidate does not claim SRTP support.
Select microphone and voice output in the Audio panel before registration. Those WASAPI endpoint IDs are fixed for the session; stop/re-register to change them.
Without explicit selections, Windows default communications capture/playback devices are used.
Missing selected endpoints block new calls/answering; reconnect or stop and choose another endpoint. Ring preview remains independent.
Physical headset audio, bidirectional RTP and actual Digitro PBX calls require acceptance testing.
Third-party license texts are included under licenses/.
The installer and executable are unsigned. File hashes identify artifacts, not publisher certification.

During an established native call, MUTE changes the SDK microphone flag; HOLD/RESUME requests local hold state. A local hold flag is not confirmation of remote SIP negotiation. Validate microphone silence and peer hold/resume behavior on the target PBX/headset. For direct transfer, enter the destination during an established call, click TRANSFERIR and review it before sending. One request per call; SDK submission is not confirmed central acceptance. Attended transfer remains unavailable.
