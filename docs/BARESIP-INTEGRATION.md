# Baresip integration gate

The desktop currently selects `UnavailableSipEngine` using `create_sip_engine()`.
This is deliberate: neither the Baresip SDK nor a native backend is currently linked.

## Requirements before enabling Baresip
1. Pin an audited Baresip/re release and record build instructions and licenses.
2. Implement `SipEngine` on a worker thread with safe event dispatch to the Win32 message loop.
3. Handle registration, network failures, incoming/outgoing calls, hangup and shutdown.
4. Securely handle credentials and avoid storing passwords in source code or logs.
5. Verify SIP over TLS and media SRTP independently.
6. Validate Windows audio devices and codec negotiation with test PBX.
7. Keep the fallback behavior until Windows CI and real calls pass.

There must be no successful-registration or connected-call claim when using the fallback.

## Preflight v0.17.0

O aplicativo audita localmente um conjunto fixo de nomes de DLL em modules/ ao lado de baresip.exe, e mostra códigos de saída de processo. Isso prepara o diagnóstico de empacotamento, mas **não** implementa o motor SIP, o carregamento real das DLLs nem canal de controle. As chamadas permanecem bloqueadas até o backend ser integrado e validado.
