/* stub_game_render.c - versão mínima do núcleo do jogo (#3).
 * Define só o que o render.c usa, para testá-lo isolado, sem o game.c real.
 * Os valores são fixos e previsíveis, para os testes saberem o que esperar. */
#include "game.h"

/* Atributos das torres A e B, na ordem do struct: {cost, range, damage, period}.
 * Iguais aos do game.c; a legenda mostra custo, alcance e dano. */
const TowerStats TOWER_STATS[TOWER_TYPES]={{40, 1, 2, 1}, {60, 2, 5, 2}};

/* Letra de cada tipo de torre: 0 -> 'A', 1 -> 'B'. */
char game_tower_letter(int type){
    return (char)('A' + type);
}
/* Vida máxima de um inimigo: cresce 5 por onda e 1 por nível de dificuldade.
 * O render usa isso para decidir se o inimigo está ferido (2/3 da vida ou menos). */
int game_enemy_hp(int wave, int difficulty){
    return 10+5*wave+difficulty;
}
/* Quantidade de inimigos da onda: 5, 6, 7, 8, 9 (ondas 0 a 4). */
int game_wave_size(int wave){
    return 5+wave;
}
/* Total de inimigos das 5 ondas: 5+6+7+8+9 = 35 (o render não usa; faz parte do contrato). */
int game_enemy_total(void){
    return 35;
}
/* Quantos inimigos estão na trilha agora: conta os que ainda têm vida (hp > 0). */
int game_active(const GameState *g){
    int n=0;
    for(int i=0; i<MAX_ENEMIES; i++){
        if(g->enemies[i].hp>0){
            n++;
        }
    }
    return n;
}