# Inventário de dispositivos de áudio — AXE Voice

## O que a entrega faz

O botão "Verificar microfone e saída de áudio" consulta a API Windows Core Audio/MMDevice, sem conectar ao motor SIP:

1. Conta dispositivos ativos de entrada (eCapture) e saída (eRender).
2. Identifica dispositivos padrões para comunicações (eCommunications).
3. Exibe um resumo local. Na v0.25, a aba ÁUDIO também apresenta nomes amigáveis de entradas e saídas.
4. A seleção é mantida somente em memória: ao atualizar, mantém um endpoint válido ou usa o padrão de comunicação.
5. Não persiste identificadores, nomes, escolhas ou gravações.

Arquivos: src/audio_devices.h, src/audio_devices_win32.cpp, tests/test_audio_inventory.cpp.

## Teste manual

1. Execute o script scripts/build-windows.ps1 com o parâmetro -Run.
2. Conecte um headset USB.
3. Abra a aba ÁUDIO e clique ATUALIZAR DISPOSITIVOS. Verifique as listas e a marcação [PADRAO].
4. Escolha outro headset e confirme que a interface informa a falta de ligação com o Baresip.
5. Volte a Ajustes e clique em DISPOSITIVOS DE ÁUDIO.
6. Compare com Configurações do Windows > Sistema > Som.
7. Altere o dispositivo padrão, atualize a aba e verifique o resultado.
8. Desconecte o headset selecionado e atualize novamente; a escolha deve cair no padrão.

O CTest de apresentação não precisa de hardware físico no runner Windows.

## Fora de escopo

O inventário não confirma permissão de microfone, reprodução, qualidade, dispositivo selecionado pelo Baresip, codec, RTP/SRTP, registro SIP nem sucesso da chamada. Hotplug real, aplicação da seleção ao Baresip, latência, eco e áudio bidirecional ainda precisam de homologação.
