# Native candidate0.58 — duração local da chamada

A build nativa mostra DURACAO HH:MM:SS abaixo do status SIP, em todas as abas e no modo MINI. O contador começa na primeira observação local de chamada estabelecida com evidência nativa atual; não inclui discagem nem toque. Continua durante espera enquanto SIP permanece InCall. Troca de chamada/sessão, perda de evidência, parada do motor ou fim da chamada reiniciam o contador. Sem chamada válida, exibe --:--:--.

O relógio usa GetTickCount64 na interface e política pura testada, sem depender da hora civil. Amostras regressivas são limitadas ao último instante observado. Horas não voltam a zero após99. Não há histórico, gravação de número/conta nem exportação adicional; o diagnóstico continua recebendo somente indicadores booleanos. Duração não comprova RTP, qualidade de áudio, instante exato de atendimento remoto ou homologação.

Smokes verificam o estado inativo, MINI e ausência de colisão de labels. Testes puros cobrem identidade/prova, reset e extremos numéricos. O build também rejeita versões ausentes/divergentes nos metadados. O fixture TLS agora permite até2s para completar handshake e restaura polling0.2s, com teste local real que atrasa Finished0.4s. CA, hostname, zero REGISTER para peer inválido e prazos do aplicativo permanecem obrigatórios.

Chamadas na Dígitro, headset físico, INFO/URA e acessibilidade em Windows físico continuam pendentes. A release permanece candidata.
