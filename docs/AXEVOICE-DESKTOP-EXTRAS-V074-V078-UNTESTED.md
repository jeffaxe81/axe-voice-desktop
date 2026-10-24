# AXE Voice Desktop — cinco funcionalidades locais v0.74–v0.78

Desenvolvimento antecipado em 24/10/2026. **Sem testes novos e sem execução manual de testes nesta rodada**. O build e o CI não foram verificados neste ciclo; workflows existentes podem disparar automaticamente.

| Versão | Funcionalidade | Implementação |
|---|---|---|
| 0.74 | Iniciar com Windows | Opção local em MAIS, HKCU Run com caminho completo entre aspas. Desativada por padrão. |
| 0.75 | Fechar para bandeja | Preferência persistida em HKCU, desativada por padrão. X oculta quando habilitada e há ícone de bandeja; **Sair** encerra. |
| 0.76 | Favorito de discagem | Um destino validado salvo no HKCU como `FavoriteDestination` sob demanda; carrega para o discador, mas **não liga**. |
| 0.77 | Destinos recentes | Até dez destinos editados, validados, sem duplicatas, mantidos na memória da sessão. **Não são chamadas realizadas ou recebidas.** |
| 0.78 | Privacidade visual | Alterna a máscara na edição do número. Não criptografa e não oculta o destino na lista de recentes. |

Acesso pela aba Teclado, botão **MAIS** (Ctrl+G). Atalhos: Ctrl+D salvar favorito; Ctrl+F recuperar favorito; Ctrl+H recentes; Ctrl+Shift+P privacidade.

## Regras de continuidade e fechamento

- Não habilita registro SIP, chamadas, áudio de conversa, RTP/SRTP, nem desbloqueia os botões de chamada, pois não há confirmação de motor nativo pronto e seguro na GUI.
- O favorito fica em texto claro sob o registro do usuário atual do Windows; **nunca guardar senhas, tokens ou segredos como destino**.
- O histórico recente não é persistido em disco e pode ser apagado no mesmo menu.
- Executar build Windows x64, testes unitários, integração/caixa-preta, revisão da inicialização com Windows, bandeja, DPI/teclado e homologação SIP real antes de encerrar aceite de produção.
- Não confundir conclusão do código local com a homologação. A PR deve permanecer Draft até os critérios de aceite serem comprovados.
