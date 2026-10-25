# Auditoria inicial do SDK nativo — 25/10/2026

Escopo: compatibilidade do adaptador isolado com SDK real disponível, sem ativar SIP na GUI. O manifesto [NATIVE-SDK-CANDIDATE-20261025.json](NATIVE-SDK-CANDIDATE-20261025.json) registra versões, commits, hashes e limites do candidato.

## Resultado

- Baresip/libre4.12.0 disponíveis em builds Linux e LLVM-MinGW x64 UCRT. O Baresip tem modificações locais; o manifesto registra o hash do patch completo. Não é uma distribuição original sem alterações nem um SDK aprovado para MSVC.
- O probe de callbacks não compilava contra os cabeçalhos reais porque incluía baresip.h antes de re.h. A inclusão de libre agora precede Baresip, como exige a API pública. Cabeçalhos fake do teste anterior eram autossuficientes e não expunham esse erro.
- Alvos opcionais que consomem o SDK agora recebem WIN32 explicitamente, pois os cabeçalhos libre selecionam a API Winsock por esse macro. O ajuste fica restrito aos quatro alvos nativos.
- O contrato ABI, o probe de callbacks, o adaptador e o reactor compilaram contra os cabeçalhos reais. Ambos os probes ABI ligaram estaticamente com Baresip/libre, OpenSSL e bibliotecas do sistema. A dependência winpthread foi necessária para clock_gettime64 neste SDK MinGW.
- O smoke do reactor executou contra libre real no Linux: inicialização, fila, wake, timer e parada concluíram com exit0. A execução com aplicativo+libre instrumentados por ASan/UBSan e detecção de vazamentos também terminou com exit0. O OpenSSL do sistema não está instrumentado.
- A compilação completa padrão Windows x64 continua concluindo, com adaptador nativo OFF. Os probes Windows reais foram somente compilados/ligados, sem execução Windows.

## Gate preservado

O gate nativo da main exige bibliotecas MSVC .lib e hashes explícitos. Os arquivos candidatos são .a do MinGW e foram rejeitados pelo gate, conforme esperado. Os critérios não foram alterados nem substituídos por extensão de arquivo renomeada. Hashes identificam bytes fornecidos; não autenticam distribuição ou aprovam licenciamento.

A experiência de compilação MinGW é evidência de compatibilidade de cabeçalhos/símbolos nesse toolchain, não uma ativação do gate MSVC. Nenhum UA, conta, registro, chamada SIP, RTP/SRTP ou headset foi usado. A GUI mantém o motor nativo isolado e chamadas bloqueadas.

## Próximo gate

Obter ou compilar SDK MSVC x64 de origem fixada, registrar licenças e dependências transitivas, fornecer .lib e hashes corretos ao CMake e compilar os alvos opcionais pelo caminho oficial. Em seguida executar o reactor em Windows e verificar quiescência/lifetime com o SDK real antes de conectar o adaptador à GUI. A arquitetura e a política TLS/SRTP exigem validação própria antes de chamadas reais.
