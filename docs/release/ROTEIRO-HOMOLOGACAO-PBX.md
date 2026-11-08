# AXE Voice — roteiro de homologação real do escopo original

**Natureza:** documento operacional de aceite. **Nenhum cenário abaixo está marcado como aprovado.** Não executa ligações nem registra automaticamente no PBX. Escopo: Win32/Baresip como cliente SIP principal; WinUI 3 é candidato separado. Exclui expressamente #234–#238.

## Preparação e controle de dados

- Usar **dois ramais descartáveis**, central SIP de homologação autorizada, firewall conhecido e headset USB. Registrar identificadores reais somente em sistema corporativo de acesso restrito, nunca em issue pública.
- Validar o SHA-256 dos executáveis instalados, versão da central, Windows, interface de rede, transporte (UDP/TLS) e codecs. Não publicar credenciais, IPs privados, números pessoais ou capturas SIP completas.
- Fazer teste de chamada receptiva e ativa em sentidos opostos. Anotar o **resultado observado** e o **esperado**; não confundir callback do SDK ou loopback sintético com evidência de interoperabilidade.
- Em qualquer falha de registro ou áudio, cancelar a homologação do cenário correspondente e registrar incidente de investigação; não forçar um indicador verde.

## 30 cenários de aceitação (todos pendentes)

Cada cenário contém campo de evidência e aceite separado; marcar apenas após reprodução em ambiente físico. O responsável deverá registrar SHA, data, tipo de PBX, versão, resultado e referência a relatório privado, sem segredo SIP.

| ID | Cenário | Resultado esperado | Situação |
|---|---|---|---|
| H01 | Instalação Win32 limpa sem conta | Abre desconectado, sem senha gravada | Pendente |
| H02 | Registrar via UDP com usuário correto | Desafio autorizado e confirmação real 200 OK | Pendente |
| H03 | Informar senha errada | Registro falha, sem indicação conectado e sem vazamento | Pendente |
| H04 | Desligar/religar rede | Perda de evidência e recuperação por nova autenticação | Pendente |
| H05 | Renovação de REGISTER durante repouso | Registrado apenas com confirmação recente válida | Pendente |
| H06 | Diferença porta local e destino 5060 | Contact, Via, rport e INVITE direcionados corretamente | Pendente |
| H07 | DNS indisponível/intermitente | Diagnóstico sem falso registro | Pendente |
| H08 | TLS com CA confiável autorizada | Registro seguro e certificado validado | Pendente |
| H09 | TLS com CA inválida | Sem downgrade silencioso para UDP | Pendente |
| H10 | Dois aplicativos ocupando porta local | Falha de escuta reconhecida, sem falso positivo | Pendente |
| H11 | Originar chamada para ramal B | A recebe mídia e B toca, estados coerentes | Pendente |
| H12 | Chamada de A para B não atendida | Ringback e timeout corretos | Pendente |
| H13 | CANCEL antes de atender | B interrompe toque; A não exibe chamada estabelecida | Pendente |
| H14 | BYE originado em A | A/B encerram estados e recursos | Pendente |
| H15 | BYE remoto de B | A encerra e libera mídia | Pendente |
| H16 | Chamada recebida A ← B | INVITE real chega, aparece toque e botão de atender | Pendente |
| H17 | Atender chamada recebida | 200 OK/ACK, estado em chamada coerente | Pendente |
| H18 | Rejeitar chamada recebida | Resposta SIP correta; sem mídia ativa | Pendente |
| H19 | CANCEL remoto enquanto toca | Chamada desaparece sem resíduo no Monitor | Pendente |
| H20 | Novo INVITE após encerramento | Token de chamada antigo não bloqueia novo atendimento | Pendente |
| H21 | G.711 A-law | Codec negociado e áudio bidirecional real | Pendente |
| H22 | G.711 µ-law | Codec negociado e áudio bidirecional real | Pendente |
| H23 | Alternar seleção manual/automática de codec | SDP anuncia apenas módulos disponíveis | Pendente |
| H24 | Chamada bidirecional de 60 s | RTP em ambos sentidos, sem mudo involuntário | Pendente |
| H25 | Remover/reconectar headset USB | Comportamento recuperável, sem travamento | Pendente |
| H26 | Trocar entrada/saída WASAPI | Estado real de áudio e dispositivo indicado | Pendente |
| H27 | DTMF (INFO) e espera/retomada | Respostas/interoperabilidade conforme PBX | Pendente |
| H28 | Transferência permitida pelo PBX | Confirmação efetiva, sem concluir por mera solicitação | Pendente |
| H29 | Interface Win32 e WinUI 3 em 100/125/150/200% | Sem sobreposição, captura real comparada com referência | Pendente |
| H30 | Instalar, atualizar, desinstalar e revisar licenças | Atalho e payload removidos, rollback e SBOM aprovados | Pendente |

## Registro de resultado por cenário

Preencher no repositório privado de homologação, sem colar dados do cliente no GitHub público:

```text
ID: H__
Versão AXE Voice / SHA-256:
Edição: Win32 | WinUI3 LAB
Windows e versão de firmware/central:
Data da execução:
Transporte e codec (somente categoria):
Passos reproduzidos:
Observado:
Resultado: PASSOU | FALHOU | BLOQUEADO
Referência ao relatório restrito:
Responsável pelo aceite:
```

## Critérios de liberação

**GO** para produção exige H01–H30 aprovados ou exceções formalmente assinadas e tecnicamente justificadas, revisão de proveniência/licenças (#130), assinatura e SmartScreen, resultado final do instalador e consentimento da equipe responsável. Qualquer falha em recepção SIP, registro ou áudio bidirecional implica **NO-GO**.

Conectado ao plano geral [ACEITE-ESCOPO-ORIGINAL.md](ACEITE-ESCOPO-ORIGINAL.md), à [matriz de 100 tarefas](https://github.com/jeffaxe81/axe-voice-desktop/issues/241) e à investigação de recepção [#216](https://github.com/jeffaxe81/axe-voice-desktop/issues/216). Isto é um roteiro, não evidência de execução.
