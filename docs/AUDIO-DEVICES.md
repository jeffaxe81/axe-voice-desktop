# Inventário de dispositivos de áudio — AXE Voice

## O que a entrega faz

O botão "Verificar microfone e saída de áudio" consulta a API Windows Core Audio/MMDevice, sem conectar ao motor SIP:

1. Conta dispositivos ativos de entrada (eCapture) e saída (eRender).
2. Identifica dispositivos padrões para comunicações (eCommunications).
3. Exibe um resumo local. Na v0.25, a aba ÁUDIO também apresenta nomes amigáveis de entradas e saídas.
4. A seleção é mantida somente em memória: ao atualizar, mantém um endpoint válido ou usa o padrão de comunicação.
5. Não persiste identificadores, nomes, escolhas ou gravações.

Arquivos: src/audio_devices.h, src/audio_devices_win32.cpp, tests/test_audio_inventory.cpp.

## Saídas independentes para toque e conversa (v0.49)

A aba **ÁUDIO** agora apresenta três seleções independentes, somente para
pré-visualização e futuras configurações do motor SIP:

- **Microfone:** dispositivo Windows de captura, preferindo o padrão de comunicação.
- **Voz da conversa:** dispositivo Windows de reprodução, preferindo o padrão de comunicação.
- **Toque de chamada:** dispositivo Windows de reprodução, preferindo o padrão de multimídia
  do Windows (`eMultimedia`). Pode ser diferente da voz da conversa.

É possível escolher **toque no computador e voz no headset** ou **toque no
headset e voz no computador**. Os IDs dos endpoints e escolhas ficam apenas em
memória; nenhum dado é salvo em arquivo, enviado pela rede ou gravado em log.
Ao atualizar o inventário, escolhas presentes são mantidas; um dispositivo
desconectado cai no padrão correspondente e não mantém o ID antigo.

**Limite fundamental:** esta entrega é uma interface de preferências locais,
não um controle efetivo do Baresip. Não é produzido toque pelo app e nenhuma
chamada/RTP é roteada. A aplicação real aos módulos de áudio do Baresip e a
homologação manual com headset precisam vir depois da integração SIP.

Teste de mesa: conecte um headset e confirme nas três listas que pode alternar
entre alto-falantes e headset sem mudar a outra saída. Remova o dispositivo
selecionado, atualize e confira o fallback. A CI cobre os dois sentidos da
seleção, o caso de mesma saída, padrões de comunicação/multimídia e hotplug
simulado sem depender de hardware físico.

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
