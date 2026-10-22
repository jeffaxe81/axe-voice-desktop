# Controle local e parada observável (v0.20.0)

O Baresip de laboratório recebe somente o comando fixo `/quit\\n` por um pipe anônimo privado (stdin) no Windows, sem API exposta na rede e sem credenciais. A herança é limitada aos handles explicitamente permitidos por `PROC_THREAD_ATTRIBUTE_HANDLE_LIST`.

Na v0.19, o botão **Parar motor** faz apenas a solicitação pelo pipe e retorna ao loop de mensagens da interface. O timer da UI consulta o estado do processo a cada 100 ms; quando observa a saída, habilita novamente os botões. Após 1500 ms, se ainda estiver ativo ou a consulta falhar, solicita encerramento forçado do Job Object. Um segundo comando `/quit` no mesmo processo é recusado; reiniciar durante a parada fica bloqueado.

**Complemento v0.20:** `request_quit_async()` duplica o handle privado do pipe e agenda uma thread Win32 que envia os seis bytes de `/quit`. A UI não executa `WriteFile`. O trabalhador não acessa o objeto `WindowsProcess` e fecha seu próprio handle ao concluir. O evento `WM_CLOSE` adia a destruição da janela até a saída observada do motor ou até a contingência de encerramento forçado após 1,5 segundo.

**Limites:** enfileirar não significa que o Baresip recebeu ou interpretou `/quit`. Os testes integrados ainda utilizam somente subprocessos sintéticos, sem registro SIP ou áudio. O fallback `TerminateJobObject` e sua espera de até cinco segundos ainda são síncronos e podem bloquear a UI em situações excepcionais. Não há garantia formal sobre a latência dessas chamadas Windows, nem canal de resposta do Baresip.


A política de timeout é testada de forma determinística em `test_shutdown_policy`. O teste Windows `test_windows_process` comprova solicitação única, saída natural de processo de teste e fallback forçado.

Referência do projeto Baresip: https://github.com/baresip/baresip/wiki/Commands-registry
