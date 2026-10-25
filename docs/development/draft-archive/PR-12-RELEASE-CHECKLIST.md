> **CÓPIA DOCUMENTAL HISTÓRICA DA PR #12.** Versão extraída da branch `feature/baresip-desktop-bootstrap`. Não é registro de merge do código nem evidência atual de compilação/testes; o texto abaixo pode conter afirmações históricas que precisam de revalidação. A `main` não ganhou estas funcionalidades. Consulte a [matriz das 13 PRs](README.md).

# AXE Voice v0.16 — release/homologation checklist

## Five incremental software deliveries
1. Human-readable SIP status labels.
2. Declared codec capabilities; G.729 disabled until external support and licensing validation.
3. Fixed diagnostic codes without credentials.
4. Explicit call-action policy (no dialing unless registered).
5. Release gate and validation checklist.

## Strict release gates
- [ ] Windows GitHub Actions build and tests pass for the **final** commit.
- [ ] AXE-Voice.exe opens on Windows 10/11.
- [ ] SIP server and account input validation passes.
- [ ] Baresip pinned version and license inventory approved.
- [ ] Registration passes with a disposable SIP test account.
- [ ] Outgoing and incoming calls confirmed on two endpoints.
- [ ] Bidirectional audio verified with headset.
- [ ] No SIP password or sensitive diagnostic contents stored in logs.
- [ ] TLS/SRTP and codec interoperability tested against target PBX.

Until these are complete, the executable is a prototype GUI, **not** an operational softphone.
