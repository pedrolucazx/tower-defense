#include "game.h"
#include <stdlib.h>
#include <string.h>

#define KILL_SCORE 10
#define VICTORY_BONUS 100

const TowerStats TOWER_STATS[TOWER_TYPES]={
    {40, 1, 2, 1},
    {60, 2, 5, 2}
};

/* Retorna a letra correspondente ao tipo da torre. */
char game_tower_letter(int tipo){
    if(tipo == 0){
        return 'A';
    } 
    if(tipo == 1){
        return 'B';
    } 
    return '\0';
}

/* Calcula a vida de um inimigo conforme a onda e dificuldade. */
int game_enemy_hp(int onda, int dificuldade){
    if(onda<1 || onda>MAX_WAVES || (dificuldade != 0 && dificuldade != 1)){
        return 0;
    }
    return (16*onda-6)*(dificuldade+1);
}

/* Calcula a quantidade de inimigos de uma onda. */
int game_wave_size(int onda){
    if(onda<1 || onda>MAX_WAVES){
        return 0;
    }
    return 4+4*onda;
}

/* Calcula o total de inimigos das cinco ondas. */
int game_enemy_total(void){
    int total=0;
    for(int w=1; w<=MAX_WAVES; w++){
        total+=game_wave_size(w);
    }
    return total;
}

/* Conta os inimigos vivos. */
static int game_alive_count(const GameState *g){
    int count=0;
    for(int i=0; i<MAX_ENEMIES; i++){
        if(g->enemies[i].hp>0){
            count++;
        }
    }
    return count;
}

/* Procura o primeiro espaço livre no vetor de inimigos. */
static int game_free_enemy_slot(const GameState *g){
    for(int i=0; i<MAX_ENEMIES; i++){
        if(g->enemies[i].hp <= 0){
            return i;
        }
    }
    return -1;
}

/* Inicializa completamente uma partida. */
void game_init(GameState *g, int dificuldade){
    if(g==NULL){
        return;
    } 
    memset(g, 0, sizeof *g);
    board_init(&g->board);
    g->hp=INITIAL_HP;
    g->money=INITIAL_MONEY;
    g->wave=1;
    if(dificuldade == 0 || dificuldade == 1){
        g->difficulty=dificuldade;
    } else {
        g->difficulty = 0;
    }
    g->status=PLAYING;
}

/* Cria uma partida utilizando alocação dinâmica. */
GameState *game_create(void){
    GameState *g=malloc(sizeof *g);
    if(g==NULL){
        return NULL;
    }
    game_init(g, 0);
    return g;
}

/* Verifica se a partida continua em andamento. */
int game_active(const GameState *g){
    return g != NULL && g->status==PLAYING;
}

/* Compra e posiciona uma torre. */
bool game_buy(GameState *g, int linha, int coluna, int tipo){
    if(g == NULL || !game_active(g)){
        return false;
    }
    if(tipo<0 || tipo>=TOWER_TYPES){
        return false;
    }
    if(g->tower_count<0 || g->tower_count>=MAX_TOWERS){
        return false;
    }
    int cost=TOWER_STATS[tipo].cost;
    if(g->money<cost){
        return false;
    }
    char letter=game_tower_letter(tipo);
    if(!board_place(&g->board, linha, coluna, letter)){
        return false;
    }
    Tower *tower=&g->towers[g->tower_count];
    tower->row=linha;
    tower->col=coluna;
    tower->type=tipo;
    tower->cooldown=0;
    g->tower_count++;
    g->money -=cost;
    return true;
}

/* Gera um inimigo na primeira posição disponível. */
static void game_spawn_enemy(GameState *g){
    int slot=game_free_enemy_slot(g);
    if(slot < 0){
        return;
    }
    Enemy *enemy=&g->enemies[slot];
    enemy->hp=game_enemy_hp(g->wave, g->difficulty);
    enemy->step=0;
    g->spawned++;
}

/* Verifica se um inimigo está dentro do alcance quadrado. */
static int game_in_range(const Tower *tower, const Enemy *enemy, const Board *board){
    if(enemy->step<0 || (size_t)enemy->step >=board->path_length){
        return 0;
    }
    Position p=board->path[enemy->step];
    int row_distance=abs(tower->row - p.row);
    int col_distance =abs(tower->col - p.col);
    int range=TOWER_STATS[tower->type].range;
    return row_distance<=range && col_distance<=range;
}

/* Executa os ataques de todas as torres. */
static void game_attack(GameState *g){
    for(int t=0; t<g->tower_count; t++){
        Tower *tower=&g->towers[t];
        const TowerStats *stats=&TOWER_STATS[tower->type];
        if(tower->cooldown>0){
            tower->cooldown--;
            continue;
        }
        int target=-1;
        int greatest_step=-1;
        /* Escolhe o inimigo mais avançado dentro do alcance. */
        for(int i=0; i<MAX_ENEMIES; i++){
            Enemy *enemy=&g->enemies[i];
            if(enemy->hp<=0){
                continue;
            }
            if(!game_in_range(tower, enemy, &g->board)){
                continue;
            }
            if(enemy->step>greatest_step){
                greatest_step=enemy->step;
                target=i;
            }
        }
        if(target<0){
            continue;
        }
        Enemy *enemy=&g->enemies[target];
        enemy->hp -=stats->damage;
        tower->cooldown=stats->period-1;
        if(enemy->hp<=0){
            memset(enemy, 0, sizeof *enemy);
            g->kills++;
            g->money +=REWARD;
            g->score +=KILL_SCORE;
        }
    }
}

/* Movimenta todos os inimigos vivos. */
static void game_move_enemies(GameState *g){
    for(int i=0; i<MAX_ENEMIES; i++){
        Enemy *enemy=&g->enemies[i];
        if(enemy->hp<=0){
            continue;
        }
        enemy->step++;
        /* O inimigo chegou à base. */
        if((size_t)enemy->step>=g->board.path_length-1){
            memset(enemy, 0, sizeof *enemy);
            g->escaped++;
            if(g->hp > 0){
                g->hp--;
            }
        }
    }
}

/* Atualiza ondas, vitória e derrota. */
static void game_update_status(GameState *g) {
    if(g->hp<=0){
        g->hp=0;
        g->status=DEFEAT;
        return;
    }
    if(g->spawned !=game_wave_size(g->wave)){
        return;
    }
    if(game_alive_count(g) != 0){
        return;
    }
    if (g->wave==MAX_WAVES) {
        g->status=VICTORY;
        g->score+=VICTORY_BONUS;
        return;
    }
    g->wave++;
    g->spawned=0;
    g->wave_turn=0;
}

/* Executa um passo completo da partida. */
void game_tick(GameState *g){
    if(!game_active(g)){
        return;
    }
    
    /* 1. Geração de inimigos. */
    if(g->spawned < game_wave_size(g->wave) && g->wave_turn %3==0){
        game_spawn_enemy(g);
    }
    /* 2. Ataques das torres. */
    game_attack(g);
    /* 3. Movimentação dos inimigos. */
    game_move_enemies(g);
    /* 4. Atualização dos turnos. */
    g->turn++;
    g->wave_turn++;
    /* 5. Verificação do resultado da partida. */
    game_update_status(g);
}

/* Valida a consistência completa da partida. */
bool game_valid(const GameState *g){
    if(g ==NULL){
        return false;
    }
    if(g->board.path_length == 0){
        return false;
    }
    const int total=game_enemy_total();
    /* Verificação dos limites gerais. */
    if(g->hp<0 || g->hp>INITIAL_HP || g->money<0 || g->tower_count<0 || g->tower_count>MAX_TOWERS || g->wave<1 ||
        g->wave>MAX_WAVES || g->spawned<0 || g->spawned>game_wave_size(g->wave) || g->wave_turn<0 ||
        g->wave_turn>MAX_TURNS || g->turn<0 || g->turn>MAX_TURNS ||
        g->kills<0 || g->kills>total ||
        g->escaped<0 || g->escaped>total || g->kills+g->escaped>total ||
        g->score<0 || g->score>KILL_SCORE*total+VICTORY_BONUS ||
        (g->difficulty !=0 && g->difficulty !=1)){
        return false;
    }
    if(g->status !=PLAYING && g->status !=VICTORY && g->status !=DEFEAT){
        return false;
    }
    
    /* Reconstrói o tabuleiro esperado. */
    Board expected;
    board_init(&expected);
    int spent=0;
    for(int i=0; i<g->tower_count; i++){
        const Tower *tower = &g->towers[i];
        if(tower->type<0 || tower->type >=TOWER_TYPES){
            return false;
        }
        const TowerStats *stats =&TOWER_STATS[tower->type];
        if(tower->cooldown<0 || tower->cooldown>=stats->period){
            return false;
        }
        if(!board_place(&expected, tower->row, tower->col, game_tower_letter(tower->type))){
            return false;
        }
        spent +=stats->cost;
    }
    if(memcmp(&expected, &g->board, sizeof expected) !=0){
        return false;
    }
    /* Confere dinheiro e pontuação. */
    int expected_money=INITIAL_MONEY+REWARD *g->kills-spent;
    int expected_score=KILL_SCORE *g->kills;
    if(g->status==VICTORY){
        expected_score +=VICTORY_BONUS;
    }
    if(g->money != expected_money || g->score !=expected_score){
        return false;
    }
    /* Confere a vida conforme as fugas. */
    int expected_hp =INITIAL_HP-g->escaped;
    if(expected_hp < 0){
        expected_hp=0;
    }
    if(g->hp !=expected_hp){
        return false;
    }
    /* Confere os inimigos vivos. */
    int alive=0;
    for(int i=0; i<MAX_ENEMIES; i++){
        const Enemy *enemy=&g->enemies[i];
        if(enemy->hp < 0){
            return false;
        }
        if(enemy->hp==0){
            if(enemy->step !=0){
                return false;
            }
            continue;
        }
        if(enemy->step<0 || (size_t)enemy->step >=g->board.path_length-1){
            return false;
        }
        if(enemy->hp>game_enemy_hp(g->wave, g->difficulty)){
            return false;
        }
        alive++;
    }

    /* Uma onda totalmente gerada e sem inimigos vivos já deveria ter avançado. */
    if(g->status==PLAYING && g->spawned==game_wave_size(g->wave) && alive==0){
        return false;
    }

    /* Calcula quantos inimigos já foram gerados. */
    int generated=g->spawned;

    for(int w=1; w<g->wave; w++){
        generated +=game_wave_size(w);
    }
    if(g->kills+g->escaped+alive !=generated){
        return false;
    }
    /* Confere a coerência do estado da partida. */
    if(g->status==PLAYING && g->hp==0){
        return false;
    }
    if(g->status==DEFEAT && g->hp !=0){
        return false;
    }
    if(g->status==VICTORY){
        if(g->wave !=MAX_WAVES || g->spawned !=game_wave_size(MAX_WAVES) || alive != 0 || g->hp<=0){
            return false;
        }
    }
    return true;
}