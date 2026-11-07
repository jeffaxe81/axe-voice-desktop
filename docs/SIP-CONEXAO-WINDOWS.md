# AXE Voice — diagnóstico de conexão SIP (Windows)

Este roteiro separa **compilação com motor SIP real** de **interface apenas**. Ele não exige e não deve registrar credenciais em repositório, relatório ou captura de tela.

## 1. Escolher o artefato correto

- **`AXE-Voice-Windows-UI-Only-No-SIP`**: compilação Win32 de interface e testes. Usa `UnavailableSipEngine` e **não registra ramal**.
- **`AXE-Voice-Native-Candidate-Setup`**: instalador de testes com Baresip SDK nativo (perfil TLS/media). Este é o pacote destinado aos testes de registro e chamadas. Ainda não homologado em produção.
- A fonte de verdade dos artefatos é o [GitHub Actions do repositório](https://github.com/jeffaxe81/axe-voice-desktop/actions).

Se a interface mostrar **"Esta compilacao nao inclui motor SIP"**, o problema é o pacote instalado, não a senha do ramal.

## 2. Configurar a conta

1. Em **Servidor SIP**, insira somente o hostname DNS ou IP: exemplo `pbx.exemplo.com.br`. **Não** digite `https://`, `http://` nem barras de endereço web. O endereço do portal HTTPS pode não ser o endpoint SIP.
2. Preencha **Usuário/ramal** e **senha** somente no aplicativo. Não envie senha por chat, imagem, log ou issue.
3. Para um entroncamento/registro negociado em **UDP 5060**, selecione UDP e porta **5060** como porta **remota** do registrar. A porta SIP local pode ser dinâmica.
4. Não marque **TLS** em uma conta definida para UDP. TLS só deve ser usado quando a central realmente oferecer SIP/TLS e a cadeia de certificados estiver corretamente configurada.
5. Use o botão **Registrar** e observe o indicador de status real. O botão ou a abertura do programa, isoladamente, não confirmam registro.

## 3. Identificar a classe de falha

| Evidência | Interpretação e próxima checagem |
|---|---|
| **Motor SIP indisponível** | Executável UI-only; instalar candidato nativo |
| **Falha de configuração** | Corrigir hostname, usuário, porta, transporte ou certificado |
| **REGISTER sem resposta / prazo esgotado** | Revisar DNS, firewall, rota, NAT e porta UDP do registrar |
| **401 Unauthorized** inicial | Desafio SIP Digest normal; deve ser seguido de novo REGISTER com Authorization |
| **401 repetido ou 403** | Investigar usuário de autenticação, senha, política da central e realm, sem publicar segredos |
| **200 OK** válido após REGISTER | Registro aceito; ainda verificar renovação, recebimento de chamadas e áudio |
| **Registrou mas não recebe chamadas** | Avaliar Contact/roteamento do registrar, NAT, keepalive e alcance do destino de sinalização |
| **Chamada sem áudio** | Avaliar SDP, RTP, NAT, firewall, codec e dispositivo de áudio; registro SIP não prova mídia |

**Atenção:** `Test-NetConnection host -Port 5060` testa **TCP**, não valida UDP/5060.

## 4. Evidências para diagnóstico sem segredos

No monitor de SIP, registrar horário da tentativa, hostname mascarado quando necessário, transporte, status do motor, estágio/erro SDK e se houve resposta do registrar. Não compartilhar `Authorization`, `Proxy-Authorization`, senha, tokens ou dumps completos sem sanitização. Para análise controlada, capturar tráfego com filtro Wireshark `sip || udp.port == 5060` e remover identificadores/credenciais antes de compartilhar.

## 5. Testes automatizados

- A matriz `tests/run_1000_native_sip_connections.py` usa o executável real `axv_native_sip_engine_smoke.exe` contra um **registrar local em 127.0.0.1**, nunca contra uma central externa.
- São quatro cenários (`success`, `wrong-password`, `timeout`, `missing-audio`), com 250 testes de cada em execução padrão.
- O relatório `AXE-Voice-Native-SIP-1000-Results` contém o resultado detalhado em JSON.
- **Execução de referência:** [GitHub Actions 42345278699](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/42345278699), commit `63278874`, **1000/1000 aprovados**, 0 falhas. O resultado não equivale a homologação na central real.

## 6. Aceite para utilização real

O aceite ainda requer registro na central pretendida, renovação do REGISTER, chamada de saída, recepção de INVITE, atender/encerrar, DTMF e áudio bidirecional com headset real, com verificação de logs sanitizados e segurança antes de qualquer uso em produção.
