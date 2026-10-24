# AXE Voice Desktop — checkpoint técnico v0.13.0 (áudio)

**Data:** 18 de outubro de 2026  
**Escopo:** software proprietário independente do MicroSIP/GPL; este repositório público preserva somente histórico/documentação e não recebe o código-fonte proprietário.

## Entrega privada realizada

- Partindo do pacote privado de código v0.12.0, foi preparada a versão-fonte **v0.13.0**.
- Corrigido o envio do destino SIP da transferência na interface Win32: encaminhar uma `std::string` completa (antes, a expressão usava apenas o primeiro caractere).
- Criado `BridgeSessionFence` com geração monotônica de conexão para descartar mensagens atrasadas e desconexões de sessões anteriores.
- O cliente Windows vincula filas e comandos à geração da sessão conectada; uma autorização antiga da UI não pode atravessar reconexões.
- Adicionado teste unitário portátil para transições de sessão, gerações inválidas, duplicatas e eventos tardios.

## Evidências

| Verificação | Resultado | Limite |
| --- | --- | --- |
| Baseline v0.12.0: CTest Linux Release | 21/21 aprovados | Sem SIP real |
| v0.13.0: CTest Linux Release | 22/22 aprovados | Lógica/mock |
| v0.13.0: Clang ASan/UBSan | 22/22 aprovados | Lógica/mock |
| Compilação MSVC x64 e GUI Win32 | Não executada | Necessária |
| Baresip autenticado, chamadas SIP, áudio RTP/SRTP | Não executados/implementados | Bloqueio de produção |

**Atenção:** a cerca de geração protege o estado e a fila contra eventos atrasados, mas **não autentica o processo remoto**. O Baresip `ctrl_tcp` permanece inseguro, utilizável somente no laboratório isolado com `AXV_UNSAFE_CTRL_TCP_LAB=ON`. O build padrão Windows segue sem controle de telefonia, e `AXV_COMMERCIAL_RELEASE=ON` permanece bloqueado.

## Próximos critérios de aceite

1. Compilar e executar o host em Windows 10/11 x64 com Visual Studio 2022; validar Full/Mini/tray e ações de transferência, chamadas e reconexões sem crash.
2. Desenvolver o controlador do Baresip com IPC local autenticado, identidade verificável, ACLs adequadas e sem porta `ctrl_tcp` insegura paralela.
3. Testar cadastro/registro SIP, chamadas bidirecionais RTP, controles DTMF, transferência, mute e espera em PABXs reais.
4. Revisar licenças e SBOM de Baresip/libre/codecs; verificar assinaturas e cadeia de distribuição antes de qualquer release comercial.

**Não distribuído:** executável Windows comercial, motor Baresip, instalador assinado ou código-fonte privado. **Fases futuras:** vídeo, chat, Web e mobile; não incluídos no MVP de áudio.
