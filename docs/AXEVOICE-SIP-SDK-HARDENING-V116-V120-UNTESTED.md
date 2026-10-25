# AXE Voice LAB — ciclo v0.116–v0.120 (sem execução de testes)

Data: 24/10/2026. Base: desenvolvimento v0.115. Todos os itens abaixo são de **implementação de código não compilada nem homologada nesta rodada**. O GitHub poderá iniciar workflows por gatilhos próprios; isto não substitui evidência.

## Cinco entregas

| Versão | Mudança implementada | Limite |
|---|---|---|
| v0.116 | **Evitar registro fora do controlador**: o parser de perfil Baresip passa a rejeitar `module account`, `module account.so`, `module account.dll` ou `module account.dylib`, além de caminhos redirecionados e linhas ambíguas. Essa aplicação pode carregar contas de arquivo e disparar `ua_register` automaticamente. | Verificação sintática estática do perfil; uma auditoria completa dos módulos carregados ainda é necessária. |
| v0.117 | **Auditoria da fonte da senha SDK**: o opt-in `AXV_EXPERIMENTAL_NATIVE_WINCREDS_LAB` exige `AXV_BARESIP_ACCOUNT_SOURCE` e `AXV_BARESIP_ACCOUNT_SOURCE_SHA256`, um arquivo `account.c` com SHA256 aprovado e correspondente exatamente ao build da biblioteca. Sem fonte e hash, CMake recusa a habilitação. | Hash de fonte não prova que biblioteca instalada foi produzida dela. Também são exigidos pins existentes do cabeçalho e biblioteca Baresip/libre e revisão manual da proveniência. |
| v0.118 | **Decodificação WinCred mais precisa**: aceita ASCII ou UTF-16LE representando ASCII e um único terminador nulo final opcional; rejeita NUL interno, bytes de controle, Unicode não representável e tamanhos inválidos; sem conversão com perdas. | Não suporta todas as senhas Unicode; nenhum segredo sai por relatórios ou logs. |
| v0.119 | **Memória da conta Baresip**: leitura limitada a 256 caracteres para zerar a senha antes de liberar o campo nativo; comparação de conteúdo após a instalação para detectar cópia não fiel do SDK, sem publicar o valor nem sua assinatura. | A gravação por ponteiro retornado como `const char*` continua condicionada à revisão do código-fonte/ABI do SDK pinado; a destruição de uma string inesperadamente maior não comprova apagamento seguro. |
| v0.120 | **Controlador de registro fail-closed**: recusa timeout fora dos limites da fila, não penaliza orçamento de login por entrada errada e exige conta preparada, geração atual, reactor funcionando e sem falhas antes de solicitar registro. A versão CMake, janela LAB e diagnóstico local foram alinhados para 0.120. | SDK nativo continua isolado, sem `SipEvidence::VerifiedNativeApi`; chamadas no frontend continuam bloqueadas. |

## Fundamentação da API

A [API pública do Baresip](https://github.com/baresip/baresip/blob/main/include/baresip.h) expõe `ua_account`, `account_set_auth_pass` e `account_auth_pass`. A [implementação pública](https://github.com/baresip/baresip/blob/main/src/account.c) no momento da leitura usa alocação com `str_dup` e libera com `mem_deref`; porém **a versão local do SDK deverá corresponder aos arquivos e hashes aprovados**, não simplesmente à branch `main` do repositório público. A política de apagamento de cópias transientes precisa de uma revisão independente.

## Gatilhos e riscos pendentes

- `AXV_BUILD_NATIVE_BARESIP_ADAPTER=OFF`, `AXV_BUILD_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME=OFF` e `AXV_EXPERIMENTAL_NATIVE_WINCREDS_LAB=OFF` permanecem desligados por padrão.
- O executável `AXEVoice.exe` **não é linkado ao SDK nativo**. Sua implementação mantém `UnavailableSipEngine`, autorização e evidência de chamada em modo negado.
- Nenhuma chamada SIP real, áudio RTP/SRTP, login autenticado, assinatura ou instalador de produção foi gerada.
- A lib precisa de compilação MSVC Windows com SDK pinado, verificação ABI e evidência de autenticação 401/407/200 com PABX real, TLS de servidor confiável, SRTP, headset/PC e chamadas bidirecionais.
- As mudanças são adequadas apenas à continuidade na **branch de desenvolvimento**. Não mesclar `main` ou declarar GO operacional.

**Testes e builds intencionalmente não executados nesta rodada.** A revisão estrutural/compare de commits não certifica compilação nem segurança.
