# AXE Voice — callback lifetime gate (laboratório)

The v0.39 work adds a bounded native callback lifecycle boundary in
`src/baresip_callback_lifecycle.h`. It builds and runs in the Windows CI without
the Baresip SDK. **It does not register any Baresip callback or provide SIP calls.**

## Why this is necessary

In a future in-process Baresip integration, a callback may remain in flight
while the user stops or restarts the engine. A callback tied to an older process
must not enter a newer SIP session, nor access destroyed native resources.

## Contract

- `begin()` creates a monotonically numbered generation and a separate mailbox
  session. Captured callbacks must retain **both IDs**, not look up current ones.
- `registration()`, `call_started()` and `call_changed()` admit events only
  from the currently open generation and then use the existing callback ingress.
- Every admitted callback owns a short-lived RAII lease. `revoke_and_wait()`
  closes admissions, invalidates all pending ingress events, then waits for the
  admitted callbacks to release those leases.
- The adapter MUST unregister the actual Baresip native callback **before**
  calling `revoke_and_wait()`. It MUST keep native resources and the lifecycle
  object alive until native deregistration and quiescence are both complete.
  Call `revoke_and_wait()` from a management thread, never from a callback.
- Start and stop are serialized. Reuse of tokens from retired generations is
  rejected. A malformed event revokes the ingress; the integration must
  explicitly call `revoke_and_wait()` before creating a new generation.
- No native proof is issued by this layer. `VerifiedNativeApi` remains reserved
  for an independently audited in-process adapter. The Windows UI continues to
  pass `SipEvidence::None`, with dial controls blocked.

## Verification

`baresip_callback_lifecycle` exercises lease admission and release, closure
before drain, restart, concurrent late callbacks, call correlation, an invalid
call key and explicit recovery. It uses synthetic events only. A successful CI
is not evidence of SIP, RTP/SRTP, headset integration or real Baresip callbacks.

## Remaining gates

Pin and audit Baresip/re binaries, source and licenses; implement real callback
registration/unregistration with safe ownership; verify real registration and
call events; handle account secrets; validate TLS/SRTP and headset audio against
a test PABX; complete Windows 10/11 manual checks before leaving Draft.


## Integração experimental: lease durante callback inteiro

O método `hold_native_callback()` valida época e sessão e mantém um lease desde a entrada no `bevent_h` até o retorno completo de `on_event`. Antes, os leases existiam somente ao publicar mensagens no mailbox, o que deixava trechos do callback fora da barreira de quiescência. `stop()` continua dependendo de `bevent_unregister` e de sincronização externa do event-loop nativo antes da destruição dos contextos de callback.

`revoke_and_wait()` libera temporariamente o mutex de controle de geração enquanto espera os leases, preservando `current_` como ocupado. Isso evita deadlock caso um callback já admitido entre em `fail_closed()->reject_current()` durante o encerramento. O teste fake-SDK bloqueia um callback durante consulta à UA, inicia `stop()` em outra thread e verifica que a parada não conclui até o callback ser liberado. **Não é validação com SDK Baresip real e não habilita SIP ou mídia na GUI.**

O campo `registered_` agora e marcado como falso somente **depois** do retorno de `revoke_and_wait()`. Isso elimina uma escrita concorrente enquanto um callback admitido ainda pode consultar `active()`. O contrato com o event-loop real continua exigindo serializacao de start/stop e barreira de registro do SDK.
