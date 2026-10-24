# AXE Voice v0.65 — conciliação do adaptador de eventos Baresip

Base: v0.64 (PR #79). Mantém as salvaguardas recentes da interface: rejeição de eventos SIPSESS globais não atribuíveis, identidade de chamada por geração, prevenção de ponteiros reutilizados e exclusividade do handler do Baresip.

## Implementação
- O callback `bevent` adquire uma **lease externa** antes de interpretar argumentos do SDK.
- O encerramento desregistra o handler e aguarda as leases em voo terminarem ANTES de liberar contexto, sessão e ponteiros nativos.
- `REGISTER_OK` só é aceito após `ua_isregistered(selected_ua)` concordar com o callback. Contradição invalida toda a sessão.
- Chamadas recebidas e originadas exigem conta ainda registrada. `native_register_ok_seen()` retorna apenas observação de controle sob a thread nativa, sem conceder acesso SIP à GUI.
- Teste `baresip_sdk_callback_quiescence` pausa o fake SDK durante um callback e valida que a parada fica bloqueada até o callback sair, impedindo reuso de sessão antiga.
- Testes anteriores foram adaptados para usar o estado nativo simulado, preservando os testes de eventos atribuídos e não atribuídos.

## Pendências de homologação
A aplicação `AXE-Voice.exe` continua com `UnavailableSipEngine`, `DisabledIncomingRingtoneSink`, sem SIP/RTP real. Nenhuma prova `VerifiedNativeApi` chega à interface. Antes de conectar `BaresipNativeRegistrationHost`, é necessária validação com SDK auditado, criação segura da UA, armazenamento de credenciais, PABX real e segurança TLS/SRTP.
