# Candidate0.57.1 — revisão em cinco frentes

Cinco desenvolvedores auditaram chamadas/DTMF, áudio, interface, diagnóstico e empacotamento em paralelo. Chamadas e diagnóstico não exigiram mudanças após testes focados, incluindo sanitizadores. Foram corrigidos três grupos de falhas:

- A falha ao criar o worker de toque agora limpa o pedido ativo, permitindo nova tentativa; o teste injeta falha de alocação sem ampliar a API de produção.
- Botões e checkbox TLS usam BS_NOTIFY para que foco pelo teclado acione a rolagem. Smokes padrão/nativo enviam foco ao controle real e verificam a viewport.
- O manifesto registra versão e hash do executável após smoke; o pacote compara versões e hashes do SDK, rejeita DLL extra e recria o stage. Os gates verificam arquivos recursivos, reinstalação e desinstalação. Testes negativos alteram entradas e restauram os arquivos em finally.

Hashes verificam integridade dos arquivos declarados, sem equivaler a assinatura ou atestado de origem externa. O instalador continua candidato e sem assinatura. Dígitro, URA/INFO, headset físico e acessibilidade em Windows físico permanecem pendentes.
