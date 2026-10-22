# AXE Voice — Frontend Windows v0.22 (prototipagem)

## Telas

**Teclado:** marca AXE Voice, indicação de conta SIP não registrada, campo editável para destino, teclado 12 teclas, comandos Apagar/Limpar e CTA de Chamar explicitamente desativado. Os botões permitem montar um ramal; o campo aceita também entrada manual com URI SIP. Não existe discagem automática.

**Ajustes:** servidor, usuário, porta e seleção TLS (apenas validação local), consulta de dispositivos Core Audio, inspeção dos módulos Baresip e comandos de iniciar/parar motor de laboratório com confirmação. O aplicativo não persiste senha SIP nem efetua registro.

**Aparência:** alternância claro/escuro, opção FIXAR/SOLTAR para manter a janela acima das demais. Janela compacta, sem instalador e sem execução em segundo plano. Somente estas preferências de interface são persistidas no Registro do usuário em HKCU\\Software\\AXEVoice\\Desktop; não há armazenamento de credenciais.

**Controles de chamada:** botões Chamar, Mudo, Espera, Transferir e Encerrar são visíveis porém indisponíveis, para não simular funções de telefone inexistentes.

**Acessibilidade:** Tab e Shift+Tab navegam pelos campos e botões; Ctrl+K abre o teclado, Ctrl+L foca o destino, Ctrl+, abre Ajustes, Ctrl+T alterna o tema, Ctrl+P fixa/solta. Labels permanecem acessíveis como controles Win32. Leitores de tela e zoom precisam de validação manual.

## Critérios automatizados

- Teste de modelo de teclado: mapeamento 12 teclas, backspace, rejeição de metacaracteres e limite de caracteres.
- CMake/MSVC e suíte CTest no CI.
- Evidência manual ainda necessária: renderização real em Windows 10/11, zoom/DPI, navegação por Tab, compatibilidade com leitores de tela e contraste visual.

## Limites e segurança

Interface gráfica concluída apenas como protótipo. O botão de chamada permanece desativado até haver registro SIP real, circuito de mídia, TLS/SRTP conforme política e homologação. A presença do processo Baresip não deve ser apresentada como registro.

O tema e a opção sempre-no-topo são persistidos exclusivamente no registro HKCU do usuário; não há telemetria, registro SIP nem envio de credenciais.
