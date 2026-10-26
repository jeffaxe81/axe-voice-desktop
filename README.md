# Integração dos pull requests na main

Integração solicitada pelo titular. A main recebe o código dos PRs existentes, mantendo o histórico e os limites abaixo. Este merge não é uma declaração de homologação SIP, áudio ou produção. A descrição histórica do bootstrap está em [PR12](docs/integration/PR12-BOOTSTRAP-README.md).

# AXE Voice Desktop

> **Status de integração — 25/10/2026:** a `main` inclui o candidato Win32/Baresip e instalador nativo. A build padrão continua sem SDK; use o pacote nativo candidato para validar telefonia. As verificações automatizadas de Windows/SDK/instalador não substituem o aceite na central Dígitro e com headset físico, mantido pendente. Consulte o [checklist atual](docs/RELEASE-CHECKLIST.md), o [roadmap](docs/ROADMAP.md) e os [critérios de promoção](docs/AXEVOICE-RELEASE-PROMOTION.md).

Projeto para pesquisa, compilação, customização e evolução controlada de um softphone SIP desktop baseado no MicroSIP/PJSIP.

## Status

**Fase atual:** P1.1 — Fundação do Projeto  
**Estado:** inicialização do repositório  
**Produção:** não

Neste primeiro ciclo, o objetivo é estabelecer uma base reproduzível e versionada antes de importar ou modificar o código-fonte do MicroSIP.

## Objetivos

- reproduzir o build do MicroSIP original;
- manter uma baseline rastreável do upstream;
- implementar white label de forma incremental;
- simplificar a experiência do usuário;
- preparar configuração e provisionamento SIP corporativo;
- evoluir integrações sem comprometer a estabilidade da telefonia.

## Princípios

- microentregas pequenas e testáveis;
- nenhuma credencial real no repositório;
- mudanças controladas por Git;
- documentação e testes acompanhando cada etapa;
- preservação dos avisos de copyright e das obrigações de licenciamento;
- nenhuma customização funcional antes da validação do build original.

## Estrutura planejada

- `src/` — código-fonte do produto;
- `assets/` — recursos visuais;
- `installer/` — empacotamento/instalação;
- `scripts/` — automação de desenvolvimento e build;
- `tests/` — testes e validações;
- `docs/` — arquitetura, build, segurança, SIP e decisões;
- `LICENSES/` — inventário e textos de licenças aplicáveis.

## Branches

- `main` — baseline estável;
- `develop` — integração das microentregas;
- `feature/*` — funcionalidades;
- `fix/*` — correções;
- `docs/*` — documentação.

## Próximos marcos

1. Concluir a fundação do repositório.
2. Definir e documentar a versão baseline oficial do MicroSIP.
3. Preparar o ambiente de compilação.
4. Compilar e executar o MicroSIP original sem alterações.
5. Validar registro SIP e chamadas.
6. Iniciar white label.

## Segurança

Não versionar senhas, tokens, certificados privados, chaves privadas, credenciais SIP reais ou arquivos locais contendo segredos.

## Licenciamento

O projeto deverá validar e documentar as licenças do MicroSIP, PJSIP, codecs e demais componentes antes de qualquer distribuição. Nenhum aviso legal do upstream deverá ser removido sem base jurídica/técnica documentada.
