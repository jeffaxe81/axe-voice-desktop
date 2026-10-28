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

`CloseApplications=no` está configurado e não foi encontrado um `AppMutex`
correspondente na aplicação. O teste atual só inicia a GUI depois de instalar.
Executar uma prova comportamental em Windows com o app aberto, inclusive durante
uma chamada, registrando bloqueios de arquivos, cancelamento, códigos de saída e
eventual necessidade de reinicialização. Avaliar fechamento coordenado ou recusa
antes de modificar arquivos; não encerrar chamadas à força como solução padrão.

Estas constatações vêm da inspeção do código. Ainda não há prova executada em
Windows desta atualização A→B nem exploração demonstrada por carregamento de DLL
remanescente.
