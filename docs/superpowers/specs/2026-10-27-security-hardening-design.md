# Candidate0.61 — segurança nas fronteiras de entrada e distribuição

A solicitação é continuar novas funcionalidades com quatro especialistas de testes simultâneos, priorizando segurança. Mantemos C++17/Win32, SDK fixado e quatro investigações independentes. A execução autônoma e integração após verificações já foram solicitadas na conversa.

## Escolha de implementação
A rodada entrega proteções nas fronteiras existentes: preservar intenção explícita SIP/SIPS em normalização; recusar transporte incompatível com o motor configurado; restringir URI nativa a512 bytes e caracteres suportados; alinhar usuário da conta ao mesmo alfabeto ASCII nativo; conferir hashes de caminhos literais no pacote. É uma escolha técnica desta rodada. Reescrever o motor ou habilitar SRTP sem requisitos da implantação ampliaria indevidamente o escopo.

SIPS explícito nunca pode virar UDP. O motor TLS também não aceita SIP explícito; ramais sem esquema seguem a conta. Controles, escapes percentuais e parâmetros digitados não chegam à chamada nativa. O limite512 vale para a URI final, incluindo rota adicionada. Usuários suportados: letras ASCII, números, _, ., -, +. Não relaxar validação TLS/certificados, sessão ou identidade da chamada.

Os quatro especialistas fornecem regressões de credenciais/diagnóstico, entradas SIP, autorização e pacote. Falha de autorização não foi demonstrada; incluir matriz de regressão com mutação para garantir que as cercas existentes permaneçam. Conferência de nomes com colchetes deve verificar o próprio arquivo, não outro selecionado por wildcard. Testes Release precisam de verificações sempre ativas.

Não afirmar pentest completo, ausência de vulnerabilidades, assinatura do instalador, SRTP ou homologação de Dígitro/headset. A regressão de ciclo nativo reproduziu retenção da cópia pendente da senha em falha anterior à autenticação. Apagar essa cópia no início de fail(), antes de publicar falha; exigir teste de módulo sintético que falha antes de conta/registro.
