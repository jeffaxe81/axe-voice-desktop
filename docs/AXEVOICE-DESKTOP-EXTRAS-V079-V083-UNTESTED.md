# AXE Voice Desktop — ciclo de continuidade v0.79–v0.83

Data: 24/10/2026. Cinco evoluções em código, **sem criar nem executar testes nesta rodada**, conforme instrução de continuidade. Não houve verificação de compilação Windows ou aceitação de hardware neste ciclo. A CI existente pode ser acionada pelo GitHub independentemente.

| Versão | Entrega | Observações |
|---|---|---|
| 0.79 | Iniciar na bandeja | Quando já habilitado o início com Windows, o usuário pode optar por iniciar minimizado; HKCU Run recebe `--tray`. Caso a bandeja falhe, a janela fica visível. O argumento é literal. |
| 0.80 | Privacidade ampliada | Ao ativar o modo privado, o histórico da sessão é apagado; novos destinos não entram no histórico enquanto ativo. Favoritos/recentes ficam indisponíveis nesse modo. O campo fica mascarado, mas **isso não criptografa dados**. |
| 0.81 | Remover favorito | Comando explícito apaga o valor de destino favorito armazenado em HKCU. Exibe falha se o Registro negar a operação. |
| 0.82 | Recuperar último destino | Ctrl+Shift+R repõe o último destino validado do histórico efêmero; somente prepara o discador e não inicia uma chamada. |
| 0.83 | Copiar diagnóstico anonimizado | Comando no menu MAIS copia para a área de transferência apenas versão LAB, estado local do processo, quantidade de dispositivos, sinalizadores visuais e negação explícita de SIP/RTP homologado. Não inclui número, URI, usuário, senha, IP, nomes de dispositivos nem caminhos. |

## Interface e segurança
- Utilização das cinco funções na GUI Win32 existente, pelo menu MAIS e, quando aplicável, atalhos.
- A versão do projeto CMake foi ajustada para 0.83.0. A versão não significa release assinada/homologada.
- Não se adicionou motor SIP ao executável nem se alterou `UnavailableSipEngine`. Chamadas não são desbloqueadas sem prova real de registro e mídia.
- Inicialização automática opera apenas no Registro do usuário atual do Windows e mantém opt-in.
- Preferências antigas são preservadas, salvo os recentes (apagados quando o modo privado é ativado).
- A leitura de favorito persistido da v0.76 continua sendo texto claro. Não armazenar segredos.
- O diagnóstico no clipboard passa a existir fora do app após a cópia; o usuário deve apagá-lo quando necessário.

## Pendência de aceite
Compilar no Windows 10/11, validar comportamento de tray/boot, clipboard e modo privacidade; repetir testes unitários, integração, caixa-preta e SIP com PABX real. **Nenhuma dessas atividades foi marcada como concluída nesta rodada**. Manter `main` e release comercial sem alterações até homologação.
