# Recuperação de credenciais da GUI — candidate0.65.2

Continuar a segurança com quatro auditorias paralelas, usando C++17/Win32 existentes. A tela lê a senha antes de validar e preparar objetos que alocam memória; uma exceção pula a limpeza e escapa do manipulador. A assinatura nativa recebe senha por valor, e outros argumentos temporários também podem falhar antes do guard interno.

Extrair a ação de registro para src/native_registration_ui.h sem alterar sua semântica inicial, permitindo reproduzir falhas em código usado pela GUI com doubles Win32. Testar falhas antes de modificar o comportamento. Depois preparar conta/CA/áudio antes de ler a senha; proteger buffers wide/UTF8 por escopo; conter exceções com resultado fixo; limpar o campo após leitura ou erro excepcional. Preservar o campo em formulário normalmente inválido. Somente configurar e registrar em sequência quando aceitos, sem novas chamadas de rede no teste.

Abordagens: adicionar guards apenas ao handler deixa os temporários da chamada sem cobertura; extrair fluxo com argumentos preparados e escopos de limpeza oferece regressão comportamental. Escolha do executor: extração pequena, API de resultado enum e controles HWND. A cópia da senha para o parâmetro por valor ocorre após preparar todos os demais argumentos const-ref, sem temporários alocadores ao lado. Evitar mover SSO e prometer limpeza de sua capacidade não ativa.

Limites: não afirmar eliminação universal de cópias do Windows/compilador nem vazamento rotineiro observado. Inventário de atualização A→B e proteção durante aplicativo ativo ficam registrados para próxima implementação segura, sem exclusão genérica de arquivos do usuário. Dígitro/headset, assinatura e SRTP permanecem pendentes.
