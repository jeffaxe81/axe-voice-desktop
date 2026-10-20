# Inventário de dispositivos de áudio — AXE Voice

## O que a entrega faz

O botão "Verificar microfone e saída de áudio" consulta a API Windows Core Audio/MMDevice, sem conectar ao motor SIP:

1. Conta dispositivos ativos de entrada (eCapture) e saída (eRender).
2. Identifica dispositivos padrões para comunicações (eCommunications).
3. Exibe um resumo local; não persiste nomes, identificadores ou gravações.

Arquivos: src/audio_devices.h, src/audio_devices_win32.cpp, tests/test_audio_inventory.cpp.

## Teste manual

1. Execute o script scripts/build-windows.ps1 com o parâmetro -Run.
2. Conecte um headset USB.
3. Clique em "Verificar microfone e saída de áudio".
4. Compare o resumo com Configurações do Windows > Sistema > Som.
5. Altere o dispositivo padrão para comunicações e clique novamente.
6. Desconecte o headset e repita.

O CTest de apresentação não precisa de hardware físico no runner Windows.

## Fora de escopo

O inventário não confirma permissão de microfone, reprodução, qualidade, dispositivo selecionado pelo Baresip, codec, RTP/SRTP, registro SIP nem sucesso da chamada. Hotplug, seleção de headset, latência, eco e áudio real ainda precisam de homologação.
