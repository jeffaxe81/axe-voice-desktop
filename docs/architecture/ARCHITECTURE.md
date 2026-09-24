# Arquitetura Inicial

## Visão
O desktop será inicialmente uma customização controlada do MicroSIP para Windows.

## Princípios
- preservar o comportamento SIP do upstream;
- reduzir alterações invasivas;
- separar branding, configuração e lógica;
- preparar evolução futura para provisionamento;
- não acoplar o produto a um único PABX quando isso puder ser evitado.

## Visão futura
AXE Voice Desktop -> SIP/PJSIP -> PABX/SBC

Provisionamento e integrações serão avaliados em fases posteriores.
