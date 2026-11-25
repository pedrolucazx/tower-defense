/* stub_game_render.c - versão mínima do núcleo do jogo (#3).
 * Define só o que o render.c usa, para testá-lo isolado, sem o game.c real.
 * Os valores seguem as mesmas fórmulas do game.c (ondas de 1 a MAX_WAVES),
 * para os testes esperarem exatamente o que o núcleo real devolve. */
#include "game.h"

/* Atributos das torres A e B, na ordem do struct: {cost, range, damage, period}.
 * Iguais aos do game.c; a legenda mostra custo, dano, área (a partir do alcance)
 * e cadência de tiro (a partir do período). */
const TowerStats TOWER_STATS[TOWER_TYPES]={{40, 1, 2, 1}, {60, 2, 5, 2}};

/* Letra de cada tipo de torre: 0 -> 'Y', 1 -> 'Z'; tipo inválido -> '\0'. */
char game_tower_letter(int type){
    if(type == 0){
        return 'Y';
    }
    if(type == 1){
        return 'Z';
    }
    return '\0';
}

/* Vida máxima de um inimigo: (12*onda - 2) * (dificuldade + 1), com onda de 1 a MAX_WAVES.
 * O render usa isso para decidir se o inimigo está ferido (2/3 da vida ou menos).
 * Onda ou dificuldade fora do intervalo devolvem 0, como no game.c. */
int game_enemy_hp(int wave, int difficulty){
    if(wave < 1 || wave > MAX_WAVES || (difficulty != 0 && difficulty != 1)){
        return 0;
    }
    return (12*wave - 2) * (difficulty + 1);
}

/* Quantidade de inimigos da onda: 4 + 4*onda (8, 12, 16, 20, 24 para as ondas 1 a 5).
 * Onda fora do intervalo devolve 0. */
int game_wave_size(int wave){
    if(wave < 1 || wave > MAX_WAVES){
        return 0;
    }
    return 4 + 4*wave;
}

/* Total de inimigos das 5 ondas: 8+12+16+20+24 = 80 (o render não usa; faz parte do contrato). */
int game_enemy_total(void){
    int total=0;
    for(int w=1; w<=MAX_WAVES; w++){
        total += game_wave_size(w);
    }
    return total;
}

/* Diz se a partida está em andamento (status PLAYING), como no game.c.
 * Não conta inimigos: o render conta os vivos por conta própria. */
int game_active(const GameState *g){
    return g != NULL && g->status == PLAYING;
}
