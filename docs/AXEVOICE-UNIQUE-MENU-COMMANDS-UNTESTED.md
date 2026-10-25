# AXE Voice LAB — prevenção de colisões de comandos Win32 (não testado)

## Defeito identificado

A revisão estática da versão LAB identificou duas colisões entre comandos do menu MAIS que compartilham a mesma mensagem `WM_COMMAND`:

- `kSipPreflight = 3114` conflitava com `kCopyReadinessReport = 3114`.
- `kSipVaultStatus = 3115` conflitava com `kResetAudioSession = 3115`.

Isso pode desviar uma escolha do menu para outra rotina, devido ao processamento sequencial de `WM_COMMAND`. É especialmente importante que ações sobre o cofre de credenciais SIP não sejam confundidas com um comando de áudio.

## Correção LAB

- `kCopyReadinessReport` foi movido para **3125**; o comando pré-SIP preserva **3114**.
- `kResetAudioSession` foi movido para **3126**; o status do cofre SIP preserva **3115**.
- Adicionada verificação `static_assert` de unicidade de todos os identificadores de comandos do menu e separação da faixa dinâmica de destinos recentes. Compilações futuras devem falhar se esse conjunto voltar a conter colisões.

## Status

**Código alterado e inspecionado; sem compilar, executar testes, CI manual ou testar interações de menu no Windows.** A aplicação permanece experimental LAB, sem registro SIP/voz RTP operacional. Não há migração para `main`, nenhuma mudança no cofre SIP e nenhuma alteração em credenciais reais.

QA futuro: validar todas as opções MAIS, comandos F3/F4, teclado, a opção de redefinição de headset, eventual regressão de hotkeys e requisitos de acessibilidade no Windows 10/11.
