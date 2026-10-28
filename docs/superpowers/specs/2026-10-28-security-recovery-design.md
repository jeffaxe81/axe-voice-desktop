# Candidate0.64 — recuperação e integridade completas nas fronteiras existentes

Continuar quatro frentes paralelas de testes de segurança, preservando C++17/Win32. A escolha técnica desta rodada é tornar a configuração nativa recuperável e consistente, corrigir o diagnóstico após parar, vincular todos os arquivos SDK ao manifesto e codificar corretamente argumentos Windows. Não expandir transporte/cifra nem relaxar certificados/autorização.

A função configure_authenticated deve apagar sua senha de entrada em todas as saídas. Preparar cópias de conta, configuração e áudio antes de alterar estado; copiar senha para o slot persistente somente antes de commit sem alocações. Em exceção, apagar senha de entrada e pendente, invalidar configuração/política inativa e publicar falha recuperável com mensagem fixa. Recusar alteração com reactor ativo sem tocar na sessão existente. A garantia se aplica à senha já recebida pelo método; não afirma eliminação de cópias do chamador/sistema operacional.

Parar limpa sdk_error/sdk_stage e demais diagnósticos anteriores, retornando a estado ocioso. Configuração aceita limpa o erro antigo; uma falha SDK nova deve continuar visível.

Manifesto gerado e conferido deve cobrir todos os arquivos SDK, inclusive ocultos e licenças, excluindo somente o próprio CANDIDATE-MANIFEST.json. Essa vinculação impede arquivos adicionados após o build de entrarem no pacote sem hash; não representa assinatura nem resistência a um invasor que reescreva também a proveniência.

WindowsProcess mantém lpApplicationName explícito, sem shell e com handles limitados; argumentos são sempre codificados com regras CRT, inclusive backslash final. Teste com filho inerte valida -f com pasta de perfil contendo espaço e separador final no Windows. Caminhos não contêm credenciais.

Regressões usam valores sintéticos, sem registrar conta/chamar central. Dígitro/headset, assinatura e SRTP continuam fora do aceite desta rodada.
