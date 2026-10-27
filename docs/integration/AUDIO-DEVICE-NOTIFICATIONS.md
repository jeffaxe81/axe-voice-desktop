# Candidate0.60 — alterações de dispositivos de áudio

O observador opcional Core Audio sinaliza chegada, remoção, estado/propriedade e mudanças de padrão eMultimedia/eCommunications. Callbacks apenas alteram uma flag atômica compartilhada: não enumeram, não enviam HWND/IDs e não fazem comandos SIP. A GUI mantém COM/registro, fecha o sinal antes de cancelar registro e usa novo sinal após reinício. Falha de registro não impede fallback manual/periódico.

Na aba Áudio visível, um evento invalida consultas antigas, agrupa avisos por250ms e atualiza quando as listas estão fechadas. WM_DEVICECHANGE também sinaliza os tipos genéricos previstos, sem acessar lParam. A consulta periódica de fallback aguarda5000ms após a conclusão e não substitui consultas ocupadas, pendentes ou prontas, evitando inanição de drivers lentos. Sair/entrar em MINI cancela a geração visual. Ajustes continua manual e independente.

Notificações não alteram IDs nem rotas de chamadas; a rota nativa permanece fixa na sessão e o motor conserva a inspeção real de pré-ação. Inventário não comprova qualidade, RTP nem homologação. Registro/cancelamento COM são feitos pela thread GUI conforme contrato; não se promete prazo físico de teardown do serviço/driver.

Testes verificam sinal, política, COM simulado e integração com probe bloqueada, inclusive acima do período de fallback. Verificações permanecem ativas na build Release. Smokes não registram observador nem iniciam probes; compilação Windows real confere assinaturas/linkagem, não entrega física de hotplug. Veja a [matriz restante](REMAINING-FUNCTIONALITIES.md). Core Audio/hotplug real, acessibilidade, Dígitro/headset/INFO e troca de rota ao vivo seguem pendentes.
