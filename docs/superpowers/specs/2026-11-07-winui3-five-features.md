# AXE Voice WinUI 3 — cinco funcionalidades em cinco frentes

**Data:** 2026-11-07 · **Status:** implementação incremental em branch isolada.

## Restrições
- Manter edição Win32/engine Baresip intocados; não criar segundo motor.
- O WinUI 3 é uma prévia sem SIP ativo. Ações de chamada permanecem bloqueadas.
- Preservar design Essencial, cantos arredondados e temas claro/escuro.
- Não armazenar credenciais, nem divulgar dados de conta em diagnósticos.
- Testes Windows e WinUI 3 precisam ficar verdes antes de merge.

## Entregas (uma por frente)
1. **Validação de destino SIP:** usar a lógica C++ `evaluate_destination` exportada pela bridge read-only. Exibir erro/validade em tempo real no WinUI 3. Falha da DLL significa indisponível, nunca válido.
2. **Teclado físico acessível:** entrada via teclas 0-9, * e #; Backspace e Esc controlam o campo. Nenhuma tecla origina chamadas enquanto o motor estiver desacoplado.
3. **Favoritos locais:** até 10 destinos explicitamente favoritados, seleção/remoção, armazenamento protegido pelo usuário Windows (DPAPI) e sem envio à nuvem. Histórico de chamadas não é inventado.
4. **Monitor SIP seguro:** atualização manual do snapshot nativo, indicação de backend ausente/versão incompatível e cópia de relatório técnico *sem números, senhas, hosts ou usuário SIP*.
5. **Tamanho da interface:** opção Compacto 355×602 e Conforto 400×680, mantendo o padrão compacto, sem scroll e com leitura adaptativa.

## Aceite
- TDD C++ (bridge) + testes puros C# (discador, favoritos, relatório e tamanho).
- Microsoft Windows App SDK estável e build real WinUI 3 x64.
- Testes de falha e fallback ao desconectar bridge sem habilitar Ligar.
- QA Win32, WinUI 3, native SDK conforme pipelines disponíveis.
- Nenhuma funcionalidade é descrita como SIP operacional até testes com engine real.
