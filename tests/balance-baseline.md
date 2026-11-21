# Critérios definidos antes da simulação

Data: 2026-11-10. N=1000 por estratégia e configuração; seed inicial 42.

Critérios obrigatórios: zero partidas presas em 2000 turnos; estado válido em todas as rodadas; sem defesa perde 100%; uma sequência conhecida de posições vence; repetir semente produz CSV idêntico.

Indicadores exploratórios: comparar A, B e mista com posições aleatórias dentre células úteis (alcance cobre ao menos 6 posições da trilha). Meta inicial de 40–95% de vitória para mista; diferença acima de 20 pontos percentuais entre A/B indica investigar possível dominância. Não são prova de equilíbrio nem teste de aprovação da entrega acadêmica. Registrar falhas de meta, sem ajustar faixa depois dos resultados.

Comparar normal e pressão (vida de inimigos dobrada), mantendo mapa, economia e sementes pareadas. Reportar vitórias, derrotas, turnos, vida restante, recurso final, eliminações, ondas, min/max e erro amostral aproximado para taxa de vitória. Monte Carlo amostra decisões do bot; repetir uma estratégia inteiramente fixa 1000 vezes não produz evidência estatística nova. Testes humanos continuam necessários.
