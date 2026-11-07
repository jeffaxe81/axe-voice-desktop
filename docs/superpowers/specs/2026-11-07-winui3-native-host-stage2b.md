# AXE Voice — estágio 2B: host Baresip para WinUI 3 (candidato)

**Aprovado por direção continuada do projeto:** avançar a integração do motor SIP nativo com a segunda edição, sem afetar o Win32.

## Solução escolhida

Manter `AXEVoice.exe` e seu Baresip atuais. Construir um **segundo produto compilado** `AXEVoice.WinUI3.exe` cujo processo carrega uma variante **opcional** de `axv_frontend_bridge.dll`, ligada ao mesmo código-fonte `BaresipSipEngine`. O SDK nunca é reimplementado em C#. Cada processo possui no máximo um dono nativo da sessão e da thread Baresip.

A edição WinUI 3 padrão continuará uma **prévia offline**. A edição com backend será um **candidato de laboratório separado**, com criação de sessão apenas após clicar **Conectar**.

## Segurança/consentimento

- Nunca transportar senha SIP pelo P/Invoke C#.
- O novo host C++ deve ler **somente quando solicitado** os identificadores previamente lembrados no registro HKCU e a senha correspondente no Windows Credential Manager, conferindo `TargetName` e `UserName==servidor+"\\n"+usuário`.
- Exigir `SipTransport` explícito (`udp` ou `tls`); sem este valor recusar. Não reduzir TLS para UDP.
- Para TLS, exigir caminho CA válido e confiança Baresip, sem aceitar certificado indiscriminadamente.
- O Win32 passa a salvar transporte e CA junto dos identificadores quando usuário opta por lembrar senha; apagar junto ao esquecer.
- Evitar armazenamento de credencial em logs, relatórios e backups do aplicativo. Zerar cópias de senha após `configure_authenticated`.
- Evitar registrar automaticamente em inicialização; conexão depende de gesto explícito.

## C ABI / controles

Manter struct `AxvFrontendSnapshot` binariamente estável; atualizar versão da bridge para 2 e expor funções de `connect`, `disconnect`, `dial`, `answer` e `hangup`. Todos os comandos devem verificar snapshot exibido, identidade `session/call_id` e estado atual antes de delegar ao mesmo `VoiceApplicationService<BaresipSipEngine>`. A fila nativa repete as validações.

A variante sem SDK retorna **backend ausente**, comandos indisponíveis e nunca afirma registro.

## UI

Atualização de observação por timer discreto. Mostrar **Conectar/Desconectar** na edição candidata, enquanto não houver sessão ativa. Mostrar estados SIP reais e controles contextuais:
- Registrando: status de espera, sem Ligar.
- Registrado: Ligar quando destino validado.
- Ringing: Atender / Recusar (com tokens corretos).
- Calling/InCall: Encerrar, sem novas chamadas.
- Desconectado / SDK ausente: todas as ações de chamada desabilitadas.

## Critérios do estágio

1. Contrato ABI versionado, não quebrar o Windows CI sem SDK.
2. SDK real nativo compila host opcional no MSVC e testes offline de ausência de credenciais; nenhuma chamada externa nos testes.
3. WinUI 3 compila e empacota candidato com host nativo **separado da prévia**.
4. 1.000 testes SIP loopback continuam aprovados.
5. Só entregar como **candidato de laboratório**; não homologar sem testes com conta Dígitro e áudio bidirecional/headset.
