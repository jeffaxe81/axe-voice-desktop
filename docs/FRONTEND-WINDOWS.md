# AXE Voice — Frontend Windows v0.24 (prototipagem)

## Telas

**Teclado:** marca AXE Voice, indicação de conta SIP não registrada, campo editável para destino, teclado 12 teclas, comandos Apagar/Limpar e CTA de Chamar explicitamente desativado. Os botões permitem montar um ramal; o campo aceita também entrada manual com URI SIP. Não existe discagem automática.

**Ajustes:** servidor, usuário, porta e seleção TLS (apenas validação local), consulta de dispositivos Core Audio, inspeção dos módulos Baresip e comandos de iniciar/parar motor de laboratório com confirmação. O aplicativo não persiste senha SIP nem efetua registro.

**Aparência:** alternância claro/escuro, opção FIXAR/SOLTAR para manter a janela acima das demais, e MINI/MAX para exibir somente o campo de destino e o status SIP. Abrir Ajustes expande automaticamente a janela. Janela compacta, sem instalador e sem execução em segundo plano. Somente preferências visuais (tema, fixação, modo compacto) são persistidas no Registro do usuário em HKCU\\Software\\AXEVoice\\Desktop; não há armazenamento de credenciais.

**Segundo plano:** ao minimizar, tenta criar um ícone na bandeja de notificações. Duplo clique restaura a janela; o menu oferece Abrir e Sair. Se o shell rejeitar o ícone, não esconde a janela. A saída continua usando encerramento supervisionado do motor.

**Controles de chamada:** botões Chamar, Mudo, Espera, Transferir e Encerrar são visíveis porém indisponíveis, para não simular funções de telefone inexistentes.

**Acessibilidade:** Tab e Shift+Tab navegam pelos campos e botões; Ctrl+K abre o teclado, Ctrl+L foca o destino, Ctrl+, abre Ajustes, Ctrl+T alterna o tema, Ctrl+P fixa/solta; Ctrl+M alterna MINI/MAX. Labels permanecem acessíveis como controles Win32. Leitores de tela e zoom precisam de validação manual.

## Critérios automatizados

- O smoke test da v0.24 valida MINI/MAX, restauração automática de Ajustes, dimensões da janela e chamada desabilitada, sem gravar HKCU.
- Teste de modelo de teclado: mapeamento 12 teclas, backspace, rejeição de metacaracteres e limite de caracteres.
- CMake/MSVC e suíte CTest no CI, incluindo `ui_smoke`, que cria a janela de verdade e verifica os controles principais e a troca de abas (sem SIP e sem mostrar a janela).
- Evidência manual ainda necessária: renderização real em Windows 10/11, zoom/DPI, navegação por Tab, compatibilidade com leitores de tela e contraste visual.

## Limites e segurança

Interface gráfica concluída apenas como protótipo. O botão de chamada permanece desativado até haver registro SIP real, circuito de mídia, TLS/SRTP conforme política e homologação. A presença do processo Baresip não deve ser apresentada como registro.

O tema e a opção sempre-no-topo são persistidos exclusivamente no registro HKCU do usuário; não há telemetria, registro SIP nem envio de credenciais.

## Limitações da v0.24

A janela compacta exige revisão manual de escala DPI, leitores de tela e contraste no Windows 10/11. Não indica chamadas ativas: registro SIP e áudio reais continuam fora desta entrega; nenhuma discagem é iniciada ou armazenada.
