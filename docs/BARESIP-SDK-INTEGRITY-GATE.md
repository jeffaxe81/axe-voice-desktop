# AXE Voice — Baresip SDK pinado e validação offline (laboratório)

Esta entrega introduz o gate de integridade **apenas** para a compilação opcional
`AXV_BUILD_NATIVE_BARESIP_ADAPTER=ON`. O executável `AXE-Voice.exe` permanece
com `UnavailableSipEngine` e **nenhuma chamada SIP real** é habilitada.

## Procedimento no Windows (PowerShell)

1. Obtenha cabeçalhos e biblioteca MSVC `.lib` de um build Baresip e libre/re
   de procedência auditada, compatível com o compilador e a arquitetura.
2. Verifique origem, versões, licenças, dependências, arquitetura e cadeia de
   fornecimento **fora** deste script. Registre no inventário do projeto.
3. Fixe os hashes dos **arquivos finais** (após extração/compilação):
   `(Get-FileHash C:\SDK\include\baresip.h -Algorithm SHA256).Hash` e
   `(Get-FileHash C:\SDK\lib\baresip.lib -Algorithm SHA256).Hash`.
4. Execute CMake passando os caminhos e valores auditados. Exemplo ilustrativo
   (substituir caminhos, versão e hashes pelos efetivamente verificados):

```powershell
cmake -S . -B build-sdk -G "Visual Studio 17 2022" -A x64 \
  -DAXV_BUILD_NATIVE_BARESIP_ADAPTER=ON \
  -DAXV_BARESIP_INCLUDE_DIR="C:/SDK/include" \
  -DAXV_BARESIP_LIBRARY="C:/SDK/lib/baresip.lib" \
  -DAXV_BARESIP_VERSION="<versao-auditada>" \
  -DAXV_BARESIP_HEADER_SHA256="<sha256-64-hex>" \
  -DAXV_BARESIP_LIBRARY_SHA256="<sha256-64-hex>"
```

**No PowerShell**, use uma única linha para o comando acima ou substitua
as barras de continuação por crases (`), evitando copiar o exemplo sem ajuste.

## Condições de rejeição

O gate CMake falha de forma explícita para caminhos ausentes, biblioteca vazia
ou sem extensão `.lib`, hashes inválidos/ausentes, alterações nos arquivos,
versão do macro `BARESIP_VERSION` divergente, e cabeçalho falso localizado em
`tests/fakes/baresip.h` (inclusive se o caminho apontar por symlink).

O teste `baresip_sdk_integrity_preflight` gera arquivos sintéticos em uma
pasta temporária do build e executa **9 casos positivos/negativos**. Ele não
baixa SDK, não transmite credenciais e não usa rede/SIP. Passar nesse teste
**não atesta segurança nem autenticidade do Baresip**: os hashes somente
conferem igualdade com valores informados pelo operador, e uma biblioteca
sintética ainda pode satisfazer o teste de bytes.

## Limites e próximos gates

- A checagem não verifica assinatura digital, cadeia de custódia, SBOM,
  compatibilidade ABI, dependências transitivas, TOCTOU nem licenças.
- O `axv_native_baresip_adapter` permanece fora da interface e sem inicializar
  o motor nativo. Os CTests existentes usam mocks/dublês.
- Antes de integrar chamadas reais: revisar e fixar Baresip/re, encapsular
  o event-loop com encerramento livre de corridas, comprovar origem de eventos,
  proteger credenciais, validar SIP/TLS, RTP/SRTP e áudio duplex com headset,
  concluir homologação Windows 10/11 e distribuição assinada.

**Não remover o comportamento fail-closed ou mesclar na main antes da homologação.**


## Complemento: dependência libre/re e ABI real

A integração opcional agora exige também `re.h` e a biblioteca MSVC
`libre.lib` (ou outra biblioteca auditada compatível) com SHA-256 pinado.
O CMake recusa arquivos ausentes, vazios, hashes divergentes e os dublês
`tests/fakes`. A suíte padrão testa essas regras apenas com arquivos
sintéticos; não baixa SDK real.

O contrato C++ `src/baresip_sdk_abi_contract.h` exige as assinaturas
públicas de Baresip e libre: eventos, registro, alocação de conta,
`re_main`, `mqueue_alloc`/`mqueue_push` e demais funções. Ao ativar
`AXV_BUILD_NATIVE_BARESIP_ADAPTER=ON`, o alvo
`axv_native_baresip_abi_probe` força **linkagem real** dessas funções
contra as duas bibliotecas e compara o macro `BARESIP_VERSION`.

Forneça adicionalmente:
- `AXV_LIBRE_INCLUDE_DIR`: pasta auditada que contém `re.h`
- `AXV_LIBRE_LIBRARY`: caminho da biblioteca libre/re .lib auditada
- `AXV_LIBRE_HEADER_SHA256` e `AXV_LIBRE_LIBRARY_SHA256`: hashes reais

A linkagem poderá necessitar bibliotecas transitivas do mesmo build;
falhas são bloqueantes e não podem ser mascaradas. O executável de prova
de ABI não configura conta, inicia o loop ou registra SIP.
A suíte da CI regular usa fake headers e **não substitui** esse teste
de linkagem com SDK verdadeiro.
