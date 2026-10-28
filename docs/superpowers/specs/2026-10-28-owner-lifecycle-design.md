# Candidate0.65 — limpeza do proprietário e intenção de configuração

Continuar quatro frentes simultâneas de testes. Escolha técnica: reforçar o reactor existente e apresentar sua primeira causa de falha, sem trocar biblioteca/threading ou ampliar transporte. O callback de parada deve tolerar inicialização parcial e executar uma vez no proprietário quando o início foi tentado; falha de libre_init/fila anterior aos hooks continua sem callback.

Preservar o primeiro fault não None, incluindo quando o loop ou a limpeza também falham. O motor não pode converter encerramento por fault em parada normal: view Failed até parada explícita, com texto fixo por etapa, sem dados da conta. SDK error específico continua prioritário. Probes instrumentam apenas objetos de teste antes do loop e não enviam SIP.

Reconfiguração recusada com reactor inativo invalida toda intenção pendente anterior: apagar senha, configured=false e remover política da conta A. O cliente deve fornecer a conta desejada novamente; não registrar A silenciosamente após rejeitar B. Reactor ativo permanece intocado ao recusar. A GUI já encadeia configured&&register, portanto o reforço protege também consumidores diretos da API.

Manifesto usa caminhos relativos canônicos com '/', componentes não vazios e sem . / .., caracteres Windows reservados/controle, pontos/espaços finais, raízes, drives ou ADS. Verificar nome antes de Join-Path/hash. Manter nomes Unicode e colchetes literais válidos. É endurecimento de esquema; não foi demonstrado desvio da proveniência dos arquivos instalados por uma entrada externa.

Não alterar TLS/certificados, autorização de chamadas, rota fixa nem formato booleano de exportação. Sem assinatura/SRTP/homologação física ou alegação de pentest completo.
