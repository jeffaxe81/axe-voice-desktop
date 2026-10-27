# Candidate0.59 — consultas assíncronas de áudio

As abas Áudio e Ajustes usam workers independentes. Abrir Áudio ou pedir Atualizar agenda a consulta; em Ajustes, DISPOSITIVOS DE AUDIO solicita o diagnóstico. O timer aplica dados somente na aba correspondente, com listas suspensas fechadas em Áudio. Pedidos repetidos se agrupam em uma próxima consulta. Sair da aba ou entrar em MINI descarta a geração visual; retornar permite uma consulta nova sem aceitar dados antigos.

Cada canal executa no máximo uma probe. O worker contém somente função/dados e estado compartilhado; não recebe HWND nem objeto SIP. Encerrar/cancelar não espera Core Audio e não interrompe um driver bloqueado: a probe pode terminar depois, sem publicar dados. Sua memória é liberada ao terminar. Se a criação da thread falhar, a consulta permanece pendente e o timer limita nova tentativa. Exceções da probe viram erro de inventário.

Erros preservam IDs/listas anteriores e desabilitam escolhas/prévia até nova consulta bem sucedida. Seleções nativas explícitas ou padrão do Windows não são substituídas silenciosamente. Rotas da chamada continuam fixas na sessão; o motor mantém sua inspeção real antes de ações que criam mídia. Inventário não autoriza SIP, não comprova áudio e não implementa troca de rota ao vivo.

As dez frentes de trabalho incluem testes determinísticos de coalescência, épocas, falha de início, exceções, isolamento e destruição com probe bloqueada, além de ajuda e revisão. Smokes permanecem sem probes. Hotplug/driver real, acessibilidade e Dígitro/headset/INFO continuam pendentes. Esta é reimplementação dos requisitos documentados de PR30/v0.150–0.154, não merge do código histórico privado nem de todas as funções do arquivo histórico.
