# Auditoria local de módulos Baresip — AXE Voice v0.17.0

Recurso de diagnóstico offline do layout Windows (`baresip.exe` e subpasta `modules`). O botão **Verificar Baresip local** exibe apenas **presença de arquivos**. Não carrega os módulos nem autentica o binário.

## Mapeamento observado (nomes de DLL)

| Módulo | Alvo de produto | Observação |
|---|---|---|
| `g711.dll` | G.711 A-law/µ-law (PCMA/PCMU) | Arquivo não prova negociação SDP |
| `g722.dll` | G.722 | Arquivo não prova decodificação ou suporte no build |
| `opus.dll` | Opus | Requer runtime e dependências compatíveis |
| `wasapi.dll`, `winwave.dll` | Dispositivos Windows | Driver não é áudio duplex validado |
| `srtp.dll`, `dtls_srtp.dll` | Proteção de mídia | Não comprova SRTP ativo |
| G.729 | Etapa futura | Proibida liberação automática; exige avaliação independente |

Se a pasta `modules` não existir, o resultado será **desconhecido**: codecs podem estar vinculados estaticamente ou em outro diretório. Se houver links simbólicos/reparse points no executável ou nos artefatos inspecionados, a inicialização do laboratório é bloqueada. Isso não substitui a validação de assinatura, hash, arquitetura PE ou segurança da instalação.

## Encerramento do processo

O supervisor Windows registra `GetExitCodeProcess` apenas após detectar o encerramento espontâneo. Um encerramento solicitado pelo usuário não recebe código natural. O código não confirma sucesso de SIP nem pode ser interpretado como estado de registro.

## Testes

CTest `baresip_modules`: diretório ausente, DLLs presentes/ausentes, diretório irregular em nome de DLL.
CTest `windows_process`: saída natural em um processo filho inerte com código `7`, mantendo os testes existentes de `-f`, start/stop e reinício.

## Referências oficiais

- https://github.com/baresip/baresip/wiki/Configuration
- https://github.com/baresip/baresip/wiki/Modules
- https://github.com/baresip/baresip/blob/main/docs/examples/config

**Atenção:** não habilitar `ctrl_tcp` para suprir a integração com a interface: o módulo não oferece autenticação e sua configuração padrão pode escutar em todas as interfaces. O produto precisará de IPC local autenticado ou de embedding auditado antes de habilitar registro/chamadas.
