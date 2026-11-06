# AXE Voice — preparação de registro SIP UDP/5060

Este roteiro **não executa chamadas** e não contém credenciais, ramais reais ou host de cliente.

## Configuração manual no aplicativo (candidato nativo Windows)

1. Em **Ajustes**, digite somente o **hostname SIP** informado pelo administrador da central, sem prefixo `https://` ou barra final.
2. Informe o **usuário SIP** e a porta **5060**.
3. Mantenha **TLS desmarcado** para selecionar **UDP**. A configuração nativa gera `sip_transports udp` e `;transport=udp`.
4. Informe a senha exclusivamente no campo protegido do aplicativo. **Não** use CLI, variáveis de CI, arquivos de configuração versionados, capturas de tela nem tickets para armazená-la.
5. Selecione microfone e saída de áudio, então solicite o registro. O monitor deve distinguir entre *registro solicitado* e *registro confirmado* por evento nativo.
6. Teste primeiro uma chamada **de saída autorizada** para ramal de teste e uma chamada **entrante** de outro ramal de teste. Só amplie o volume após confirmação explícita de permissão do operador e das condições de carga da plataforma.

## Como interpretar

- Falha antes de `REGISTER`: conferir DNS, rota e firewall UDP de saída/entrada, bem como configuração do motor.
- `401 Unauthorized` durante o desafio Digest **não significa falha definitiva** se o registro autenticado subsequente receber `200 OK`.
- `403 Forbidden`: conferir permissão da conta e política do servidor (sem registrar a senha).
- `408 Request Timeout`: investigar conectividade/NAT/ACL/tempo de resposta.
- Chamada estabelecida sem áudio: verificar oferta/resposta SDP, endpoints RTP, NAT e dispositivos de áudio. A resposta SIP 200 OK, isoladamente, não comprova áudio funcional.

## Cuidados e escopo dos testes

- UDP/5060 **não oferece criptografia de sinalização**; não habilitar TLS implicitamente, mas solicitar transporte criptografado ao administrador quando disponível.
- O Monitor SIP atual registra **transições de estado**, não pacotes SIP completos. Não afirmar que mensagens `INVITE`, `ACK`, `BYE` ou RTP foram capturadas.
- Suítes automatizadas de 2.000 cenários verificam **lógica de eventos simulados**; a suíte de 1.000 registros loopback usa uma central SIP local simulada. Nenhuma delas valida por si só o servidor externo.
- Nunca configurar carga de 1.000 chamadas reais em ambiente SaaS sem autorização formal, limites de simultaneidade e de taxa, janela de manutenção e proteção de terceiros.
- A conta real deve permanecer fora do repositório e da telemetria, inclusive nos logs de falha.

## Evidências mínimas de homologação

Registrar somente: horário, resultado, evento SIP **sem cabeçalhos sensíveis**, ID de execução gerado localmente, latência de registro, evento de toque, aceite, fim da chamada e verificação de áudio pelo operador. Evitar números de telefone, cabeçalhos de autenticação, URIs com identificação e conteúdo de voz.
