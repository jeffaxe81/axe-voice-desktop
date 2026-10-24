# AXE Voice — Checklist de homologação física e SIP (não executada)

**Estado inicial:** todos os itens abaixo estão **PENDENTES**. Não
interpretar aprovação de CTest sem hardware como aceite de telefonia.

| Caso | Pré-condição | Ação / resultado esperado | Evidência |
|---|---|---|---|
| H-01 | Windows 10/11, alto-falante e headset USB | Toque no PC, voz no headset; ouvir tom de TESTAR em destino exato | Filmagem + logs locais sem IDs |
| H-02 | Mesmos periféricos | Inverter PC/headset; toque só no headset, voz só no PC | Filmagem |
| H-03 | Headset Bluetooth conectado | Testar perfis de áudio e headset, sem redirecionar silenciosamente | Medição de driver |
| H-04 | Reproduzindo tom local | PARAR VOZ/TOQUE interrompe áudio e aguarda worker | Medição acústica |
| H-05 | Toque ativo | Remover/disponibilizar dispositivo; saída anterior recebe Stop ACK | Traço de estado |
| H-06 | PABX de homologação | Login SIP com prova nativa e registro confirmado | Capture SIP |
| H-07 | Chamada recebida | Evento verificado autoriza toque; atendimento interrompe toque | Traço SIP + áudio |
| H-08 | Chamada originada | Não acionar campainha entrante; mídia duplex no destino correto | RTP/SRTP |
| H-09 | Falha de link/transporte | Revogar registro e bloquear controles até nova prova | Captura rede |
| H-10 | Fechar aplicativo | Nenhuma thread, COM, chamada ou mídia permanece ativa | Logs e processos |
| H-11 | Segurança TLS/SRTP | Protocolo, certificados, cifra e replay verificados | Auditoria |
| H-12 | Implantação Windows | Validar checksum SHA256, rollback e assinatura do pacote | Manifest + assinatura |

O manifesto de laboratório `AXE-Voice-LAB.sha256` registra apenas
a integridade do binário daquela CI; **não é uma assinatura digital**
ou certificação de interoperabilidade.
