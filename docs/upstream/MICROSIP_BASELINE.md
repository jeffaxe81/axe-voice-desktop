# Baseline Oficial — MicroSIP

## Decisão

A baseline do Projeto 1 é **MicroSIP 3.22.16.0**.

## Origem oficial

- Projeto: MicroSIP
- Site oficial: https://www.microsip.org/
- Página oficial de source: https://www.microsip.org/source
- Arquivo de source: https://www.microsip.org/download/MicroSIP-3.22.16-src.7z
- Versão do source: 3.22.16.0
- Release binária correspondente: 3.22.16
- Data da release binária: 2026-09-14
- Data de registro desta baseline: 2026-09-24
- SHA-256 do archive oficial: `9c1ff942ae1ae56bdfcdd04c3d6e242a056d7f0dd50bb19f6e13226e7c819b83`
- Arquivos extraídos: 231
- Tamanho total extraído aproximado: 2.05 MiB

## Validação da versão

O próprio `const.h` do archive oficial declara:

- `_GLOBAL_VERSION "3.22.16"`
- `_GLOBAL_VERSION_COMMA 3,22,16,0`

## Importação

O source oficial foi baixado diretamente de `microsip.org` por GitHub Actions, teve o SHA-256 calculado antes da extração e foi importado sem branding ou alteração funcional em:

`upstream/microsip-3.22.16.0/MicroSIP-3.22.16-src/`

A proveniência e o manifesto de arquivos estão em:

- `docs/upstream/MICROSIP_3.22.16.0_PROVENANCE.md`
- `docs/upstream/MICROSIP_3.22.16.0_MANIFEST.txt`

## Licenciamento

A página oficial do MicroSIP informa distribuição do source sob GNU GPL v2.

O MicroSIP depende de bibliotecas de terceiros que não estão incluídas no archive oficial. O ambiente de build deverá registrar e validar separadamente essas dependências.

O PJSIP adota licenciamento dual: GPL v2 ou posterior, ou licença proprietária alternativa. As implicações de redistribuição devem ser avaliadas antes de qualquer distribuição comercial/fechada.

## Regra de imutabilidade

1. O conteúdo em `upstream/` é referência e não deve receber customizações.
2. Qualquer alteração de branding ou comportamento será feita em uma working tree separada.
3. O primeiro objetivo é reproduzir o build original.
4. Nenhuma modernização de toolchain deve ser misturada ao primeiro build.
5. Alterações futuras devem ser comparáveis contra esta baseline.

## Tag

Tag planejada:

`upstream-microsip-3.22.16.0`

A tag deverá apontar para o commit de importação automatizada da baseline.

## Estado

- Identificação da versão: concluída.
- Origem oficial: concluída.
- Download oficial: concluído.
- SHA-256: concluído.
- Importação imutável: concluída.
- Manifesto: concluído.
- Inventário inicial de dependências: concluído.
- Tag upstream: em automação.
- Build original: próximo marco.
