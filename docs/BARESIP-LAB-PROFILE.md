# Baresip lab profile — AXE Voice 0.16.3

AXE Voice starts `baresip.exe` with `-f "<LocalAppData>/AXEVoice/baresip-lab"` on Windows. The profile directory is created on demand for the **current Windows user** and is separated from the standard Baresip profile. The GUI still asks the user to confirm execution of a locally installed, **unverified** executable.

## Safety gate (before each start)

- Reject a missing/inaccessible profile, symlinked profile, or Windows junction/reparse directory.
- Reject symlinked or non-regular `config`, `accounts`, `contacts`, and `uuid` files (including Windows reparse points).
- Refuse startup when `accounts` has any active line. Comment-only files are accepted; this intentionally prevents automatic SIP registration during laboratory bootstrap.
- Refuse startup if `config` contains an active line containing `ctrl_tcp`, `ctrl_http`, or `ctrl_ws` (unauthenticated or unevaluated remote-control modules). The check is a **guardrail**, not a full parser or guarantee against malicious configuration.
- Never pass SIP identities or passwords as process arguments; report errors without reading/printing secrets.

`-f` is a documented Baresip option: https://github.com/baresip/baresip/wiki/Configuration

## Validation

CTest `baresip_profile` checks empty profile creation, comments, configured accounts, forbidden control modules, and invalid files. Windows `windows_process` exercises `-f` with a real inert child executable and a directory containing spaces.

## Limits

This change **does not implement a SIP engine, trusted-binary verification, registration, incoming/outgoing calls, DTMF, secure credentials, media, TLS, SRTP, codecs, or authenticated IPC**. App-control modules are deliberately not activated. The UI continues to report that SIP registration is unconfirmed. The process is still forcibly stopped; use only in a laboratory until graceful call-aware shutdown exists. Directory permissions inherit the current user's profile ACLs and should be audited before credentials are stored there.

## Complemento v0.17.0

Antes de iniciar, o Windows verifica se baresip.exe e a pasta modules adjacente não são symlinks/junctions/reparse points. Bibliotecas de áudio são apenas inventariadas, sem carregamento. Diretório modules ausente não bloqueia a execução, pois o projeto pode ser estático ou ter layout diferente. O processo reporta o código de saída natural quando encerra, sem interpretar isso como registro SIP. Detalhes em [BARESIP-MODULE-AUDIT.md](BARESIP-MODULE-AUDIT.md).


## v0.33 — Regras conservadoras de inspeção de configuração

O perfil local de laboratório rejeita entradas `include`, `module_path`, `module_preload` e
`module_dir`, para impedir referências externas ainda não auditadas. A procura por
`ctrl_tcp`, `ctrl_http` e `ctrl_ws` nas linhas ativas passa a ignorar caixa
(maiúsculas/minúsculas), enquanto comentários isolados seguem permitidos.
Arquivos `config` e `accounts` acima de 128 KiB, linhas acima de 4096 bytes e
bytes NUL em linhas são recusados preventivamente.

A detecção é deliberadamente restritiva e **não** equivale a analisar toda a gramática do
Baresip, assinar módulos, autenticar o processo, validar permissões ACL ou prevenir
alteração concorrente dos arquivos (TOCTOU). A política precisa ser revista
quando o motor real, módulos e distribuição forem vinculados.
