# Agenda de contatos — candidato0.69

Dez entregas funcionais: cadastrar, editar, excluir com confirmação, buscar por nome/destino, marcar favorito, filtrar favoritos, preencher o discador, copiar destino, importar backup JSON e exportar backup JSON. A janela CONTATOS abre por botão ou Ctrl+J; é independente e não desabilita a janela principal. Usar um contato ou dar duplo clique preenche o número e mostra o discador para revisão. Somente CHAMAR solicita a chamada, pelas autorizações nativas existentes.

Os contatos ficam em `%LOCALAPPDATA%\AXEVoiceNativeCandidate\contacts.json`, separados das credenciais SIP. A agenda mantém até500 contatos, nomes até64 unidades UTF-16 e destinos até128 caracteres ASCII aceitos pelo discador. Nomes e destinos não entram no diagnóstico exportado. Copiar só modifica o clipboard quando solicitado. O backup JSON é legível e contém nomes, destinos e favoritos; não contém conta, senha, CA ou configuração de registro.

Salvar usa arquivo temporário, flush e substituição no mesmo diretório. Alterações feitas por outra instância causam conflito em vez de sobrescrita silenciosa; feche e reabra a agenda para recarregar. Arquivo inválido bloqueia novas gravações para preservar os dados. A desinstalação não apaga esta pasta de dados do usuário.

Importar abre um seletor de arquivo, valida integralmente o backup e apresenta a quantidade de novos contatos e de duplicados já existentes antes de confirmar. A importação não substitui contatos existentes; duplicatas dentro do próprio backup são rejeitadas. Arquivos acima de512KiB, esquema inesperado ou Unicode inválido são recusados. Exportar solicita destino e confirmação de sobrescrita. Favoritos aparecem antes dos demais e a busca diferencia acentos, ignorando maiúsculas/minúsculas de Latin-1.

A validação automática verifica modelos/backup, persistência e conflitos no Windows e presença/abertura/preenchimento da agenda na GUI nativa. Importação/exportação pelos seletores e acessibilidade física requerem uso no Windows do usuário. Homologação Dígitro/headset e assinatura continuam pendentes; esta entrega é uma funcionalidade local, sem nova aprovação da central ou de mídia.
