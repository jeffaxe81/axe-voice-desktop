# Próximas verificações de atualização do instalador

A auditoria desta rodada identificou uma lacuna no contrato de atualização do
candidato nativo. A correção entregue em 0.65.2 trata separadamente a limpeza de
segredos da GUI quando ocorre uma exceção; os itens abaixo continuam pendentes.

## Arquivos removidos entre versões

`scripts/package-native-candidate.ps1` reinstala o mesmo instalador no mesmo
diretório. Isso verifica uma reinstalação com o mesmo conteúdo, mas não uma
atualização da versão A para B com arquivos removidos. A limpeza do staging só
protege o pacote gerado. A seção `[Files]` do instalador sobrescreve arquivos
presentes e não remove arquivos antigos ausentes no novo pacote.

Próxima prova em Windows: instalar A com uma DLL e uma licença aninhada que B
não contém; atualizar para B e verificar os arquivos remanescentes. Incluir um
arquivo alterado entre versões, um arquivo criado pelo usuário e um arquivo
antigamente distribuído que o usuário modificou. Verificar conteúdo, remoções e
preservação após a atualização e a desinstalação.

Opções de correção a avaliar:

- Para arquivos obsoletos conhecidos, usar caminhos exatos e hashes de versões
  históricas confiáveis. Preservar ou recusar explicitamente conteúdo divergente.
- Para uma solução geral, gerar depois do staging um inventário de todos os
  destinos instalados e seus hashes. Comparar os inventários antigo e novo,
  aceitar somente caminhos relativos validados e confinados ao destino, impedir
  travessia por reparse points e preservar arquivos cujo hash diverge. A migração
  de versões antigas precisa de um inventário histórico confiável.

O `CANDIDATE-MANIFEST.json` atual descreve o SDK, não todos os arquivos
instalados: DLLs de `bin/` são achatadas no destino; há entradas do SDK não
distribuídas; CRT, README e o próprio manifesto não estão em `files`. Um manifesto
local modificável não comprova sozinho a propriedade dos arquivos. Não usar
exclusão por wildcard nem apagar recursivamente diretórios arbitrários do app.

## Atualização com o aplicativo aberto

O contrato do candidato usa marcadores recíprocos `Running` e `Setup`, no
namespace `Global`, com o SID de `TokenUser` do processo. Cada participante
publica seu marcador antes de consultar o outro e falha de forma fechada em
erros; só `ERROR_FILE_NOT_FOUND` comprova ausência. Múltiplas instâncias do app
continuam permitidas: o último handle de `Running` mantém a proteção até depois
da destruição dos runtimes nativos. O mesmo SID deve cooperar entre sessões;
outros SIDs têm escopo distinto. O `AppMutex` nativo do Inno sozinho não fecha
a corrida entre a consulta do app e a reserva do setup.

Para versões antigas sem o protocolo, a sondagem conservadora abre somente o
`AXE-Voice.exe` já existente no destino, com acesso de escrita, compartilhamento
zero e `OPEN_EXISTING`, antes de modificar a instalação ou admitir a
desinstalação. Não cria, trunca nem escreve no arquivo. Executável bloqueado ou
erro de sondagem deve causar recusa sem alterar o payload instalado e sem
encerrar o aplicativo. O handle é fechado após a sondagem: versões antigas não
respeitam a reserva e ainda podem abrir depois dela. Não há garantia contra
essa corrida de abertura do legado.

As regressões propostas cobrem marcador com arquivos desbloqueados, conflito de
compartilhamento do EXE sem marcador, GUI instalada aberta, hashes preservados
na recusa e nova tentativa após liberar a proteção. A presença desses testes
não comprova execução bem-sucedida. Executar a prova comportamental em Windows,
inclusive durante uma chamada, registrando cancelamento, códigos de saída e
eventual necessidade de reinicialização. Não encerrar chamadas à força.

Estas constatações vêm da inspeção do código. Ainda não há prova executada em
Windows desta atualização A→B nem exploração demonstrada por carregamento de DLL
remanescente. Homologação física, sessões e usuários reais distintos, assinatura
e validação Dígitro/headset continuam pendentes sem evidência separada. O contrato
detalhado e os limites estão em [NATIVE-UPGRADE-GUARD.md](NATIVE-UPGRADE-GUARD.md).
