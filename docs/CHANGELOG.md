# Changelog

## v0.28.0 — feedback de discagem em tempo real (protótipo)
- Campo de destino com validação na digitação, colagem e teclado numérico via EN_CHANGE.
- Feedback semântico de erros e aviso de bloqueio SIP; destaque suave para destino inválido.
- Guarda dupla: somente engine SIP utilizável, evento nativo comprovado e destino válido podem habilitar chamada em uma versão futura. Sem backend Baresip integrado.
- Novo teste determinístico de feedback e mais cenários no smoke da janela.


## v0.19.0 — parada responsiva do motor (laboratório)
- Solicitação `request_quit()` separada da espera pelo encerramento.
- Timer da janela supervisiona o processo a cada 100 ms, aplica 1,5 s de tolerância e desabilita start/stop durante parada.
- Novo teste de política de timeout determinístico e casos de requisição única ao filho de testes.
- Não altera registro SIP, chamadas nem mídia; Baresip real ainda exige homologação.

## Unreleased — desenvolvimento v0.16.2
### Added
- Inventário real de microfones e saídas ativos no Windows (MMDevice/Core Audio) com padrões de comunicação.
- Botão de diagnóstico de áudio no protótipo Win32 e teste determinístico sem dependência de hardware.
- Script scripts/build-windows.ps1 para configurar, compilar e testar no Windows.
- CI passa a executar o mesmo script; artifact continua restrito ao protótipo de interface.
### Not available
- Motor Baresip, registro SIP, chamadas RTP/SRTP e áudio bidirecional; homologação manual de headset pendente.

### Added
- Fundação inicial do projeto.
- README principal.
- Estrutura documental.
- Estratégia inicial de branches.
