# AXE Voice LAB — exceções em callbacks nativos Baresip (NÃO TESTADO)

## Mudança

A fronteira C do callback `BaresipSdkEventAdapter::callback` foi declarada `noexcept` e envolve admissão da lease de geração e processamento do evento em um bloco `try/catch (...)`.

Se ocorrer uma exceção C++ durante o processamento — por exemplo, falha de alocação enquanto o adaptador armazena um ponteiro de chamada já validado — a implementação tenta executar `fail_closed()`: revoga a geração do mailbox e todos os estados de registro/chamada. A lease RAII ainda é liberada durante o unwind normal da exceção. Caso também ocorra exceção ao revogar, uma barreira final impede a propagação dessa segunda falha através da ABI C.

Esta é uma **melhoria defensiva do código experimental**, não um teste de memória, prova de que a falha seria recuperável em produção ou garantia contra falhas de processo.

## Restrições

- O adaptador continua isolado em alvo opcional e desabilitado por padrão. O executável Win32 mantém `UnavailableSipEngine`; nenhuma chamada SIP ou mídia RTP/SRTP é habilitada.
- O dono do event loop Baresip deve assegurar a vida útil dos ponteiros de contexto, desempilhar callbacks antes de destruir objetos e efetuar `stop()` fora dos próprios callbacks.
- Esta alteração não incorpora as PRs históricas #31/#23/#15/#14 como um todo e não substitui a revisão de licenças, hashes e ABI do SDK.
- **Sem compilação MSVC, sem CTest/CI manual, sem injeção de falhas e sem teste em PABX** na rodada. São pendências de homologação.
