# AXE Voice v0.82 — matriz de estresse CTI e segurança

## Objetivo e escopo

Validar mais de **1.000 cenários automatizados por execução**, cobrindo
entradas, semântica de discagem, concorrência, privacidade, retenção em
memória e eventos SIP. São testes de software em ambiente Windows CI;
**não equivalem a chamadas reais de PABX, media RTP, SRTP ou headset**.

## Cenários determinísticos

| Domínio | Cenários por execução | Verificações |
|---|---:|---|
| JSON válido e normalização de destino | 200 | Destinos numéricos, SIP e deep links |
| JSON inválido e injeção de parâmetros | 200 | Campos extras, duplicados, CRLF e query smuggling |
| Idempotência e intenção de discagem | 200 | Chave repetida, destino diferente HTTP 409 na API, não duplicação, sigilo |
| Cancelamento, despacho e conclusão | 200 | Estados imutáveis e impossibilidade de concluir ou cancelar pedido errado |
| Histórico e cursores | 200 | Limites de 32 resultados e 64 eventos, expulsão e sinalização de lacuna |
| Evidência SIP, épocas e privacidade | 200 | Não confiar em SIP não verificado e não divulgar tokens nativos |
| Concorrência | 200 | 4 threads por rodada, mesmas/diferentes chaves, no máximo 1 pedido criado |
| **Total C++ do teste** | **1.400** | 800 threads criadas e encerradas |

O teste `test_local_cti_stress_matrix` devolve erro no primeiro
cenário que falhar e imprime domínio, índice e linha. Sucesso imprime
`AXE CTI MULTI-MODE STRESS PASSED: 1400/1400`.

## Testes HTTP reais adicionais

O `test_local_cti_loopback` executa **120 requisições HTTP**
contra o servidor local real (`127.0.0.1:27652`), em três famílias:
40 rotas não documentadas (404), 40 tentativas sem token (401) e
40 requisições com Origin de navegador (403). Essas requisições não
podem iniciar nem alterar uma chamada.

Os testes HTTP tradicionais também abrangem 409 por reutilização de
`Idempotency-Key` com destino divergente, replay da mesma chave e
mesmo destino, autenticação e eventos por `request_id`.

## Gates de aceitação

- Build completo Windows e todos os testes CTest existentes.
- Grupos de qualidade `unit`, `blackbox`, `integration`.
- `libre_owner_reactor`: 10 repetições consecutivas, sem falha.
- Matriz CTI: **3 execuções independentes × 1.400 = 4.200 cenários**
  adicionais, rejeitando qualquer falha intermediária.
- Native SDK ABI, TLS/mídia, compilação e instalação/desinstalação do
  candidato Windows.
- Integridade MicroSIP e CI pós-merge.

A contagem representa **cenários e repetições**, não 4.200 testes
CTest distintos ou 4.200 chamadas SIP. É cobertura ampla das áreas
selecionadas, mas não exaustiva para todos os ambientes, codecs,
operadoras, dispositivos, falhas de energia ou cargas prolongadas.

## Mudança funcional

Na v0.82, `Idempotency-Key` vincula a mesma chave ao **destino
normalizado original**. O retorno `409 idempotency_key_conflict`
protege contra mudança acidental de destino usando a mesma chave.
Nenhuma nova discagem é criada, nenhuma informação do destino
original aparece na resposta. Retenção de até 32 pedidos, sem
garantia entre reinicializações.

## Homologação ainda necessária

CRM real, usuários e agentes, central Dígitro, endpoints SIP,
atendimento, desligamento, transferência, mídia em tempo real,
permissões do Windows, headsets reais e suporte a dispositivos
precisam ser exercitados fora da CI antes de declaração produtiva.
