# Encerramento local Baresip (0.18.0)

Novo canal unidirecional **somente para encerramento**, feito por pipe anônimo do Windows. O supervisor herda exclusivamente o handle de leitura de stdin e o handle de saída NUL, via `PROC_THREAD_ATTRIBUTE_HANDLE_LIST`.

O botão Parar motor envia apenas `/quit` e aguarda no máximo 1,5 segundo, depois força o encerramento do Job Object caso ainda esteja ativo. Uma saída após o comando não comprova que o comando foi interpretado; apenas comprova o encerramento observado no Windows.

Os testes usam um processo filho inerte que aceita `/quit` e outro que ignora stdin. **Isso ainda não valida o `stdio` de Baresip em Windows**, nem o registro SIP, chamadas, DTMF, áudio ou criptografia.

A UI aguarda de forma síncrona e deverá migrar para uma tarefa assíncrona antes de uso operacional. Nenhuma credencial real é enviada pelo pipe.

Referência: https://github.com/baresip/baresip/wiki/Commands-registry