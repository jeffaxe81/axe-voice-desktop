# AXE Voice Windows — pacote de laboratório

**Finalidade:** disponibilizar uma cópia portátil do protótipo de interface Windows
para avaliação funcional. Este pacote **não é um softphone operacional**.

## O que é gerado

Após `scripts/build-windows.ps1` e `scripts/test-quality-gates.ps1`, a CI
executa `scripts/package-windows-lab.ps1`. O arquivo
`dist/AXE-Voice-Windows-Lab-v<versão>.zip` contém exclusivamente:

- `AXE-Voice.exe` — executável Win32/x64 da interface;
- `README.txt` — modo de execução e alertas de segurança;
- `SHA256SUMS.txt` — SHA-256 calculado do mesmo executável empacotado.

O teste `scripts/test-package-windows-lab.ps1` reabre o ZIP, confere que a
lista de entradas é exatamente a esperada, verifica SHA-256 entre ZIP,
manifesto e arquivo compilado, e exige os avisos do README. Ele também constrói
dois ZIPs adulterados e confirma rejeição de um checksum alterado e de um
arquivo extra (ex.: configuração acidental).

## Baixar e abrir no Windows

1. No PR do GitHub, abra o workflow **AXE Voice Windows CI** aprovado.
2. Em **Artifacts**, obtenha `AXE-Voice-Windows-Lab-Package`.
3. Extraia o ZIP e, em uma pasta local, execute
   `(Get-FileHash .\AXE-Voice.exe -Algorithm SHA256).Hash`.
4. Compare com `SHA256SUMS.txt`, então execute `AXE-Voice.exe`.

A assinatura de código **não está disponível**; o hash não demonstra autoria
ou segurança. Não contorne políticas corporativas para executar o programa.

## Limitações e critérios de liberação

- Nenhuma autenticação de ramal SIP, chamadas entrantes/saintes, áudio RTP/SRTP
  ou integração de headset em chamada real está disponível.
- Não inserir senhas reais, contas de produção, dados de clientes nem qualquer
  conteúdo sensível no protótipo de laboratório.
- Não é instalador, não exige privilégio administrativo e não faz provisionamento.
- Distribuição de laboratório não constitui aprovação de licenças, teste de
  performance, certificação, verificação em todos os Windows 10/11 ou homologação.

**Bloqueio para produção:** integração nativa com Baresip/re auditada, proteção
de credenciais, chamadas e áudio de ida e volta com PABX de testes, TLS/SRTP,
testes Windows 10/11 e revisão dos artefatos/assinatura digital.
