# AXE Voice v0.50 — Inverter saídas de voz e toque

## Funcionalidade
Na aba **Áudio**, o botão **INVERTER VOZ/TOQUE** troca as preferências
atualmente selecionadas para os dois papéis de saída:
- Voz da conversa no headset e toque no computador.
- Voz da conversa no computador e toque no headset.

A troca é permitida somente quando há duas saídas distintas **presentes
no inventário atual**. O microfone nunca é alterado. Na ausência de
headset, após desconexão ou se voz e toque apontam para a mesma saída,
o botão fica desabilitado e um comando obsoleto é recusado.

## Segurança e privacidade
- Os IDs dos dispositivos existem somente em RAM, não são registrados.
- O planejador puro não acessa Windows, perfil SIP, arquivos nem rede.
- A operação não executa som, não seleciona um dispositivo no Baresip
  e não prova permissão de uso do microfone.
- O motor `UnavailableSipEngine` mantém chamadas bloqueadas.

## Testes
- `audio_role_swap` testa inversão nos dois sentidos, saída única,
  IDs ausentes, desconexão e preferências iguais.
- `ui_smoke` verifica botão, seleção em duas combobox reais, microfone
  intacto e ausência de discagem usando **saídas sintéticas**, sem áudio.
- Testes existentes da CI (unitário, caixa-preta, integração) continuam
  obrigatórios.
- Homologação de headset USB/Bluetooth, tocando campainha e voz real
  exige Baresip nativo, ringtone player e testes Windows 10/11.
