# AXE Voice — Validação da baseline MicroSIP

A CI `AXE Voice | integridade da baseline` executa **13 testes unitários do verificador**
e compara os arquivos importados do MicroSIP 3.22.16.0 com o manifesto produzido
na importação oficial (nomes e tamanhos), sua versão, SHA-256 **documentado** do
archive original e o inventário preliminar de licenças. Um relatório JSON é
guardado como artefato por 14 dias, vinculado ao commit da execução.

**Limites importantes**: comparar nomes e tamanhos detecta muitos desvios
acidentais, mas não é uma prova criptográfica byte a byte. O SHA-256 confere a
proveniência registrada do arquivo oficial, não revalida o archive original.
Esta CI **não** compila MicroSIP/PJSIP, não faz chamadas SIP, não testa headset,
criptografia TLS/SRTP e não atesta distribuição comercial. Esses gates continuam
pendentes para P1.3 em ambiente Windows com dependências reproduzíveis.

Localmente (sem dependências extras):

```sh
python -m unittest discover -s tests -p 'test_*.py' -v
python scripts/verify_baseline.py --output evidence/voice-baseline-check.json
```

Ao finalizar, o portfólio pode mostrar **CI de verificação estática**, mas
não deve equipará-la à homologação do softphone.
