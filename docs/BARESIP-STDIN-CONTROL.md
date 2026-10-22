# Controle local e parada observável (v0.19.0)

O Baresip de laboratório recebe somente o comando fixo `/quit\\n` por um pipe anônimo privado (stdin) no Windows, sem API exposta na rede e sem credenciais. A herança é limitada aos handles explicitamente permitidos por `PROC_THREAD_ATTRIBUTE_HANDLE_LIST`.

Na v0.19, o botão **Parar motor** faz apenas a solicitação pelo pipe e retorna ao loop de mensagens da interface. O timer da UI consulta o estado do processo a cada 100 ms; quando observa a saída, habilita novamente os botões. Após 1500 ms, se ainda estiver ativo ou a consulta falhar, solicita encerramento forçado do Job Object. Um segundo comando `/quit` no mesmo processo é recusado; reiniciar durante a parada fica bloqueado.

**Limites:** envio do comando não é confirmação de que o Baresip interpretou o stdin. O teste integrado usa apenas subprocessos sintéticos, não registra SIP e não valida áudio. O fechamento da janela ainda usa uma rotina de liberação síncrona, cuja duração deve ser auditada antes de produção. `WriteFile` permanece síncrono (mensagem de 6 bytes): a parada deixa de aguardar os 1,5 s no botão, mas não oferece garantia de latência máxima do Windows nem do processo.

A política de timeout é testada de forma determinística em `test_shutdown_policy`. O teste Windows `test_windows_process` comprova solicitação única, saída natural de processo de teste e fallback forçado.

Referência do projeto Baresip: https://github.com/baresip/baresip/wiki/Commands-registry
