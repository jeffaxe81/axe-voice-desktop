# AXE Voice LAB — ciclo v0.109–v0.113: senha no objeto Baresip UA (sem testes)

Data: 24/10/2026. Base real v0.108. Esta alteração segue isolada no alvo de SDK experimental, nunca no executável do usuário.

## Cinco entregas

| Versão | Código concluído | O que NÃO comprova |
|---|---|---|
| v0.109 | `baresip_native_account_lifecycle.h`: novo ciclo **core -> ua_alloc -> instalar credencial na UA -> registro**; limpeza **após parar/unsubscribir registro e antes de destruir UA**, com rollback de todos os estágios. Mantém API legada separada. | Não testa ABI, threads, TLS ou autenticação do servidor. |
| v0.110 | `baresip_sdk_account_credentials_win32.h`: acesso efêmero à credencial genérica no Windows e instalação no campo da conta nativa via `ua_account(ua)` e `account_set_auth_pass(account, ...)`. Sem senha na AOR, config, logs, interface ou fila. | O Baresip detém uma cópia na memória enquanto a UA existe. Não equivale a SIP REGISTER aceito. |
| v0.111 | Conversão deliberadamente restrita: senha ASCII imprimível de 1–256 caracteres, aceita bytes ASCII ou UTF-16LE representando ASCII; rejeita controles, Unicode fora de ASCII e NUL. Buffers locais passam por `SecureZeroMemory`. No encerramento, a cópia da conta é apagada antes de `account_set_auth_pass(nullptr)`. | Não aceita todo formato de senha; a cópia nativa é sensível enquanto a UA está ativa. Requer auditoria do código-fonte do SDK para confirmar que o getter retorna buffer gravável e a lib não cria cópias não eliminadas. |
| v0.112 | `baresip_sdk_runtime_link_probe.cpp` ganha `static_assert` de assinaturas `ua_account`, `account_set_auth_pass` e `account_auth_pass`; build probe só é compilado quando o SDK já está habilitado. | Checagem de assinatura ainda NÃO foi compilada; não prova convenções binárias ou implementação de runtime. |
| v0.113 | Novo opt-in CMake `AXV_EXPERIMENTAL_NATIVE_WINCREDS_LAB` **OFF** adicional a `AXV_BUILD_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME` **OFF**. O runtime exige também a revisão explícita `sdk_account_copy_and_scrub_reviewed` e as revisões existentes. CMake/GUI v0.113.0. | Não libera SIP no `AXEVoice.exe` nem substitui inspeção real de fontes, hashes e dependências. |

## Fontes e limites da API

API oficial Baresip: `https://github.com/baresip/baresip/blob/main/include/baresip.h`, operações `ua_alloc`, `ua_account`, `account_set_auth_pass` e `account_auth_pass`. Implementação publicada de `account_set_auth_pass` em `https://github.com/baresip/baresip/blob/main/src/account.c` atualmente usa `mem_deref` e `str_dup`. **A versão do repositório remoto pode mudar: nunca assumir que corresponde ao SDK instalado.** Validar versão, hashes SHA256, ABI, propriedade de memória, cópias transitórias e dependências antes da ativação.

O SDK detém a credencial como memória alocada em processo, não como ponteiro do Windows Credential Manager. A versão revisada deve manter garantia de que `account_auth_pass` retorna sua alocação interna gravável, e não um bloco imutável. Em caso de diferença, manter desabilitado. Não colocar a senha em `accounts` ou AOR do perfil.

## Passos ainda pendentes para utilização real

1. Compilar a lib opcional com MSVC, SDK Baresip/libre pinados e revisar ABI/fonte, sem módulos automáticos de conta e sem controle remoto.
2. Inspecionar que o perfil isolado não inclua contas e módulos que abram canais não auditados.
3. Testar senhas válidas, ausentes, UTF-16LE, bytes malformados e falhas em `account_set_auth_pass`; confirmar limpeza em sucesso, erro, cancelamento, timeout e encerramento abrupto.
4. Validar SIP REGISTER (sucesso e 401/403), TLS e certificado, codec e SRTP em PABX real, com chamada recebida/efetuada e áudio bidirecional em headset/PC.
5. Só após isso integrar uma fonte nativa verificada à GUI, conferir auditoria, fechar PRs de homologação e liberar `main` mediante aceite.

**Não realizados neste ciclo**: build Windows, CTest, unitários, testes físicos, SIP/PABX, pacote/installer, lançamento comercial. A PR pode ser mesclada numa **branch de desenvolvimento**, sem qualquer afirmação de prontidão operacional.
