# AXE Voice — opção 1: Essencial claro

Referência visual aprovada: imagem de comparação enviada em 06/11/2026, **primeira coluna apenas**.

## Critério de fidelidade

- Interface clara, marca AXE Voice azul-marinho, fundo branco e contornos azul-acinzentados.
- Cabeçalho com nome/ícone do aplicativo e controle de tema; status real de registro SIP, ramal e indicação de conexão, nunca simular conexão.
- Campo de discagem amplo com ação apagar, teclado 3×4, 12 teclas com letras auxiliares sem sobreposição.
- Botão verde principal **Ligar**, ações secundárias **Mudo**, **Espera**, **Transferir** e navegação inferior **Teclado**, **Contatos**, **Histórico**, **Ajustes**.
- Janela compacta e fixa, sem rolagem; preservar comportamento SIP, chamadas, acessibilidade de alto contraste, diagnóstico e preferências existentes.
- Não copiar elementos das opções 2 ou 3 e não adicionar funcionalidades não solicitadas.

## Status

A alteração Win32 implementa geometria e paleta inicial da opção 1, mas **não representa ainda reprodução pixel a pixel**: o ícone/logo incorporado, pictogramas nas abas e tratamento completo dos estados ativos de chamadas exigem validação visual e funcional no Windows. O tema escolhido anteriormente permanece salvo como preferência.

## Verificações antes de main

Compilar em Windows/MSVC; executar suíte CTest, smoke da janela real, escala de DPI (100%, 125%, 150%) e comparação visual com a referência; validar registro SIP real, chamadas de entrada/saída e controle de chamada. Não declarar homologação por captura visual isolada.