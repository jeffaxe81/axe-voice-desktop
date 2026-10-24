# AXE Voice v0.62 — verificar a integridade do pacote Windows LAB

O pacote do GitHub Actions contém **três arquivos lado a lado**:

- `AXE-Voice.exe` — protótipo de laboratório Windows x64;
- `AXE-Voice-LAB.sha256` — digest SHA-256 da **mesma compilação**;
- `verify-axe-voice-lab.ps1` — script local de verificação.

Extraia o pacote em uma pasta e, **antes de executar o aplicativo**, rode
o verificador a partir do PowerShell:

```powershell
pwsh -NoProfile -File .\verify-axe-voice-lab.ps1 -Directory .
```

Se houver diferença entre o executável e o manifesto, nome alterado,
arquivo ausente ou manifesto malformado, a verificação falha. O script
**não inicia o executável**, não acessa rede e não altera configurações SIP.

O pipeline executa `lab_artifact_manifest` no binário compilado,
faz checagem básica de cabeçalhos MZ, PE x64 e PE32+, e verifica o
manifesto via sete cenários offline (original, adulteração de binário,
hash alterado, segunda linha, nome trocado e arquivos ausentes).

**Limites da evidência:** um invasor que consiga substituir **ambos**
os arquivos pode gerar outro SHA-256 válido. Portanto este mecanismo
detecta corrupção/alteração em relação ao manifesto distribuído, mas
**não substitui assinatura Authenticode, assinatura de release,
proveniência/SBOM, antivírus ou revisão de dependências.**

O protótipo LAB **não** registra contas SIP reais nem realiza chamadas
com áudio RTP/SRTP; Baresip SDK e homologação física seguem pendentes.
