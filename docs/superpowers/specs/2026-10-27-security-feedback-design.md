# Candidate0.63 — recusas compreensíveis e falhas seguras

Continuar o desenvolvimento com quatro especialistas de segurança/testes simultâneos, mantendo C++17/Win32 e execução autônoma já solicitada. Escolha técnica: ampliar as fronteiras existentes com resultado estruturado de destino e mensagem fixa, em vez de ecoar valores ou flexibilizar TLS. Preservar as funções existentes que retornam string como wrappers compatíveis.

NativeDestinationResult tem error e uri. Razões: None, InvalidAddress, TransportMismatch, UnsupportedCharacters, TooLong, InvalidAccount. Erros não contêm URI; texto depende somente do enum. Roteamento normal e limite final512 permanecem idênticos. O motor mantém último motivo sob mutex, vinculado à sessão/chamada e descartado diante de nova identidade ou outro comando admitido; configuração aceita, parar/falhar e tentativa válida o limpam. Tela de chamada mostra motivo real de recusa em vez de diagnóstico de registro; transferência mantém contexto da chamada e acrescenta a orientação do motor. F2/clipboard continuam booleanos, sem copiar motivo bruto nem destino.

Exceção na criação da thread não pode escapar do método booleano register_account: capturar sem alocar no catch e seguir limpeza existente (join, apagar senha, Failed). Informar falha de partida com mensagem fixa. Regressão usa falha de uma única alocação no teste, nunca em produção.

Manifesto deve ser objeto com tipos escalares: gui_native_sdk/product_version/product_sha256 strings, tls/production_approved booleanos, files objeto de hashes string. Validar tipos antes de comparações PowerShell que convertem coleções. Preservar o contêiner raiz e usar o tipo completo PSCustomObject real, distinguindo-o do acelerador PSObject do PowerShell. Não representa quebra comprovada de hash/binário TLS; é rejeição de proveniência malformada.

Preservar TLS/certificados, identidades, rotas e política de exportação. Sem credenciais reais, rede da central, publicação de fontes privadas, SRTP ou assinatura. Aceite físico continua pendente.
