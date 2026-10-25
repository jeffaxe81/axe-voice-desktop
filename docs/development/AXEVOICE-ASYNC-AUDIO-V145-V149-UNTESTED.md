> **RASTREABILIDADE DOCUMENTAL — NÃO EXECUTÁVEL.** A `main` não recebeu código de consulta assíncrona nem motor Baresip novo. O conteúdo da v0.149 foi mesclado exclusivamente na branch experimental de desenvolvimento, sem compilação e sem testes nesta rodada.

# AXE Voice LAB — v0.145–v0.149: consultas assíncronas de dispositivos de áudio

Data: 25/10/2026. **Nenhuma compilação, execução de testes, CTest, simulação SIP ou uso de hardware foi feita nesta rodada**, conforme orientação.

## Cinco incrementos

| Marco | Origem / implementação |
| --- | --- |
| v0.145 | Portado `AudioInventoryAsync` da [PR #30](https://github.com/jeffaxe81/axe-voice-desktop/pull/30): enumeração Windows Core Audio fora do thread de interface, um worker em andamento de cada vez, captura somente de estado compartilhado sem `HWND`. |
| v0.146 | A geração de cada consulta é invalidada na troca de aba, no encerramento da janela e quando chega uma solicitação nova; resultado antigo não é entregue ao controle visual. Exaustão do contador de geração bloqueia novas requisições, evitando reutilização. |
| v0.147 | `AudioManualRefresh` agrupa cliques e notificações enquanto um worker está ocupado. Na conclusão, a UI solicita uma única nova coleta antes de apresentar dados. |
| v0.148 | A resposta é consumida exclusivamente pelo timer da GUI e apenas com a aba de áudio visível e a prévia local interrompida. Entradas, saídas, botão trocar e botões de teste são desabilitados durante a atualização; a preferência explícita de headset/PC permanece em memória. |
| v0.149 | Atualização CMake/UI/README para 0.149, preservando os bloqueios do motor SIP não homologado. |

## Separação de responsabilidade

- O worker apenas chama `inspect_windows_audio_devices()`, que faz sua própria inicialização COM na thread auxiliar; ele nunca recebe identificadores SIP, `HWND` ou segredos.
- Nem o worker nem a notificação COM mudam a rota de mídia. O manipulador Win32 aplica uma nova lista apenas ao sair da prévia local e quando a aba de áudio estiver ativa.
- Se a enumeração falhar, os dispositivos ficam indisponíveis visualmente e preferências explícitas em RAM não são perdidas.
- Consultas antigas de gerações invalidadas são descartadas, mesmo se o driver terminar a tarefa tardiamente. O worker usa estado compartilhado próprio para não acessar a janela destruída.
- O aplicativo mantém `UnavailableSipEngine` e `SipEvidence::None`; a presença de dispositivo de áudio não comprova chamadas, codecs, autenticação SIP ou RTP/SRTP.

## Limitações e divergências

A PR #30 original continha outras funcionalidades (incluindo consulta específica na aba de Ajustes, verificação de inventário, timers e cenários de teste) que **não foram mescladas integralmente nesta reconciliação**. Outros 12 rascunhos seguem separados. Não se deve declarar os antigos como integrados apenas por esse porte parcial.

É necessário compilar no MSVC, rever o modelo de threads e os requisitos COM, testar entrada/saída/retorno à aba, troca simultânea de headset/saída padrão, desligamento com driver bloqueado e verificar latência/CPU. Isso será etapa posterior de homologação; nenhuma evidência de execução foi produzida nesta rodada.

A `main` preserva a base histórica MicroSIP/PJSIP; esta versão experimental fica na branch de desenvolvimento.
