# Proteção de atualização e desinstalação — candidate0.68

Recusar instalação/atualização/desinstalação enquanto o candidato protegido estiver aberto, sem encerrar chamadas. Impedir abertura de novo candidato durante a reserva do instalador. Preservar múltiplas instâncias do app: basta uma aberta para bloquear o setup. Escopo por usuário do token do processo (SID) no namespace Global, compartilhado entre sessões Windows; não usar USERNAME nem identidade textual não única.

Protocolo: app cria Running.<SID> e retém handle ANTES de consultar Setup.<SID>; aceita outro Running existente. Setup cria Setup.<SID> e rejeita se já existe, retém ANTES de consultar Running.<SID>. Só ERROR_FILE_NOT_FOUND indica ausência; erro/collision/acesso negado recusam. Nenhum participante pode passar ao mesmo tempo, pois cada um publica antes de sondar o outro. App guarda handle global declarado antes dos runtimes até terminar seus destructors. Setup reserva em InitializeSetup/InitializeUninstall; solta no ssPostInstall antes de [Run] e nos deinitializers idempotentes.

Nomes exatos: Global\AXEVoiceNativeCandidate.Running.<SID> e Global\AXEVoiceNativeCandidate.Setup.<SID>. App retorna170 no bloqueio e5 em erro de proteção. Smoke evita MessageBox. SDK-free GUI não participa. Instalador Inno6 tem motor32bits, mesmo instalando app64; conversão TokenUser/SID usa buffers fixos e APIsWin32, DLLGetLastError imediato.

Abordagens: AppMutex/SetupMutex nativos verificam app antes de criar setup e deixam corrida; gate exclusivo também limita instâncias. Escolha do executor: marcadores recíprocos customizados preservam comportamento existente. Sondagem conservadora de escrita exclusiva do AXE-Voice.exe no PrepareToInstall/InitializeUninstall também recusa EXE em uso de versões antigas, mas não torna essas versões participantes da reserva nem prova ausência de corrida em abertura simultânea de legado.

Não remover arquivos antigos nem modificar destino por wildcard. Não afirmar rollback atômico. Atualização A→B com remoções continua em acompanhamento. Homologação física, usuários/sessões distintos reais, Dígitro/headset, assinatura e SRTP continuam pendentes.
