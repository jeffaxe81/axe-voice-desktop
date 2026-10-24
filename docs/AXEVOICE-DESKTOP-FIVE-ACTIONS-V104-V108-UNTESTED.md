# AXE Voice LAB — cinco funcionalidades locais v0.104–v0.108

Data: 24/10/2026. Código implementado diretamente na branch de desenvolvimento da PR #89; nenhuma execução, criação ou avaliação de testes nesta rodada. Não foi realizada compilação Windows.

| Versão | Funcionalidade | Limite |
| --- | --- | --- |
| 0.104 | Copiar o destino atual para a área de transferência mediante ação explícita | exige valor válido; desabilitado no modo privado; pode ficar visível a outros programas do Windows |
| 0.105 | Colar destino Unicode da área de transferência | limite 128 caracteres; valida sintaxe local; não inicia ligação; desabilitado no modo privado |
| 0.106 | Selecionar todo o campo destino pelo menu MAIS | só manipula o controle Win32 existente |
| 0.107 | Desfazer edição com EM_UNDO do controle nativo | não altera conta SIP, áudio ou rede |
| 0.108 | Limpar destino atual e histórico efêmero com confirmação | preserva o favorito persistido; não modifica credenciais |

Implementação: `src/main.cpp` (menu MAIS e handlers Win32), `src/ui_help.h` e versão `CMakeLists.txt`.

## Segurança e limites
- `g_private_dial` bloqueia copiar/colar; informações de diagnóstico seguro não passaram a incluir números.
- O texto colado deve ser Unicode, terminado em NUL, caber no buffer Win32 e passar por `valid_dial_destination`. O programa jamais disca automaticamente.
- O item copiar depende de confirmação pela ação de menu. A área de transferência pertence ao sistema operacional e **não garante sigilo** após a cópia.
- `UnavailableSipEngine`, barreiras de evidência nativa e controles SIP/RTP permaneceram sem liberação.
- O código novo está **não compilado, não testado e não homologado**. Antes de qualquer liberação: revisão de código, build MSVC, unitários, caixa-preta, integração, testes reais de headsets e SIP/PABX.

As PRs de laboratório podem ser integradas entre branches de desenvolvimento sem publicação comercial; merge na `main` permanece pendente de homologação.
