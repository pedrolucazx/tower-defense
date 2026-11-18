#include "game.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* Procura uma posição válida para colocar uma torre. */
static int find_free_position(const Board *board, int *row, int *col){
    for(int i=0; i<BOARD_ROWS; i++){
        for(int j=0; j<BOARD_COLS; j++){
            Board copy=*board;
            if(board_place(&copy, i, j, 'A')){
                *row=i;
                *col=j;
                return 1;
            }
        }
    }
    return 0;
}

/* Testa as informações das torres. */
static void test_tower_letter(void){
    assert(game_tower_letter(0)=='A');
    assert(game_tower_letter(1)=='B');
    assert(game_tower_letter(2)=='\0');
    assert(game_tower_letter(-1)=='\0');
}

/* Testa os cálculos de vida dos inimigos. */
static void test_enemy_hp(void){
    assert(game_enemy_hp(1, 0)==10);
    assert(game_enemy_hp(1, 1)==20);
    assert(game_enemy_hp(5, 0)==74);
    assert(game_enemy_hp(5, 1)==148);
    assert(game_enemy_hp(0, 0)==0);
    assert(game_enemy_hp(6, 0)==0);
    assert(game_enemy_hp(1, 2)==0);
}

/* Testa a quantidade de inimigos por onda. */
static void test_wave_size(void){
    assert(game_wave_size(1)==8);
    assert(game_wave_size(2)==12);
    assert(game_wave_size(3)==16);
    assert(game_wave_size(4)==20);
    assert(game_wave_size(5)==24);
    assert(game_wave_size(0)==0);
    assert(game_wave_size(6)==0);
    assert(game_enemy_total()==80);
}

/* Testa a inicialização da partida. */
static void test_game_init(void){
    GameState g;
    game_init(&g, 0);
    assert(g.hp==INITIAL_HP);
    assert(g.money==INITIAL_MONEY);
    assert(g.wave==1);
    assert(g.difficulty==0);
    assert(g.tower_count==0);
    assert(g.spawned==0);
    assert(g.turn==0);
    assert(g.status==PLAYING);
    assert(game_active(&g));
    assert(game_valid(&g));
    game_init(&g, 1);
    assert(g.difficulty==1);
    game_init(&g, 5);
    assert(g.difficulty==0);
    game_init(NULL, 0);
}

/* Testa a criação dinâmica da partida. */
static void test_game_create(void){
    GameState *g=game_create();
    assert(g !=NULL);
    assert(game_active(g));
    assert(game_valid(g));
    free(g);
}

/* Testa a compra das duas torres. */
static void test_game_buy(void){
    GameState g;
    int row, col;
    game_init(&g, 0);
    assert(find_free_position(&g.board, &row, &col));
    assert(game_buy(&g, row, col, 0));
    assert(g.tower_count==1);
    assert(g.money==INITIAL_MONEY-40);
    assert(g.towers[0].type==0);
    assert(g.towers[0].cooldown==0);
    assert(g.board.towers[row][col]=='A');
    assert(game_valid(&g));

    /* Não permite comprar em uma posição ocupada. */
    assert(!game_buy(&g, row, col, 1));

    /* Não permite tipos inválidos. */
    assert(!game_buy(&g, 0, 0, -1));
    assert(!game_buy(&g, 0, 0, TOWER_TYPES));

    /* Não permite posições inválidas. */
    assert(!game_buy(&g, -1, 0, 0));
    assert(!game_buy(&g, BOARD_ROWS, 0, 0));
    assert(!game_buy(&g, 0, BOARD_COLS, 0));
    assert(game_valid(&g));

    /* Testa a compra da torre B. */
    game_init(&g, 0);
    assert(find_free_position(&g.board, &row, &col));
    assert(game_buy(&g, row, col, 1));
    assert(g.tower_count==1);
    assert(g.money==INITIAL_MONEY-60);
    assert(g.towers[0].type==1);
    assert(g.board.towers[row][col]=='B');
    assert(game_valid(&g));

    /* Não permite comprar sem dinheiro suficiente. */
    game_init(&g, 0);
    g.money=0;
    assert(find_free_position(&g.board, &row, &col));
    assert(!game_buy(&g, row, col, 0));

    /* Não permite comprar em uma partida nula. */
    assert(!game_buy(NULL, 0, 0, 0));
}

/* Testa o combate e as recompensas. */
static void test_game_combat(void){
    GameState g;
    game_init(&g, 0);
    /* Coloca uma torre B próxima ao início do caminho. */
    assert(game_buy(&g, 4, 1, 1));
    /* Primeiro turno: gera o inimigo e causa dano. */
    game_tick(&g);
    assert(g.spawned==1);
    assert(g.enemies[0].hp==5);
    assert(g.kills==0);
    /* Segundo turno: a torre está em recarga. */
    game_tick(&g);
    assert(g.enemies[0].hp==5);
    /* Terceiro turno: a torre elimina o inimigo. */
    game_tick(&g);
    assert(g.enemies[0].hp==0);
    assert(g.kills==1);
    assert(g.money==INITIAL_MONEY-60+REWARD);
    assert(g.score==10);
    assert(game_valid(&g));
}

/* Testa a movimentação dos inimigos. */
static void test_game_movement(void){
    GameState g;
    game_init(&g, 0);
    /* Prepara um inimigo no início do caminho. */
    g.spawned=1;
    g.wave_turn=1;
    g.enemies[0].hp=game_enemy_hp(1, 0);
    g.enemies[0].step=0;
    game_tick(&g);
    assert(g.enemies[0].step==1);
    assert(g.enemies[0].hp>0);
    assert(game_valid(&g));

    /* Prepara outro cenário: inimigo prestes a chegar à base. */
    game_init(&g, 0);
    g.spawned=1;
    g.wave_turn=1;
    g.enemies[0].hp=game_enemy_hp(1, 0);
    g.enemies[0].step=(int)g.board.path_length-2;
    game_tick(&g);
    assert(g.enemies[0].hp==0);
    assert(g.escaped==1);
    assert(g.hp==INITIAL_HP-1);
    assert(game_valid(&g));
}

/* Testa a passagem para a próxima onda. */
static void test_game_waves(void){
    GameState g;
    game_init(&g, 0);
    assert(game_buy(&g, 4, 1, 1));
    /*
     * Simula uma onda com sete inimigos eliminados
     * e um último inimigo vivo.
     */
    g.spawned=8;
    g.kills=7;
    g.score=70;
    g.money=INITIAL_MONEY+7*REWARD-60;
    g.wave_turn=1;
    g.enemies[0].hp=5;
    g.enemies[0].step=0;
    game_tick(&g);
    assert(g.kills==8);
    assert(g.wave==2);
    assert(g.spawned==0);
    assert(g.wave_turn==0);
    assert(g.status==PLAYING);
    assert(game_valid(&g));
}

/* Testa a condição de vitória. */
static void test_game_victory(void){
    GameState g;
    game_init(&g, 0);
    assert(game_buy(&g, 4, 1, 1));
    /*
     * Simula a quinta onda com 79 inimigos eliminados
     * e apenas um inimigo restante.
     */
    g.wave=MAX_WAVES;
    g.spawned=game_wave_size(MAX_WAVES);
    g.kills=79;
    g.score=790;
    g.money=INITIAL_MONEY+79*REWARD-60;
    g.wave_turn=1;
    g.enemies[0].hp=5;
    g.enemies[0].step=0;
    game_tick(&g);
    assert(g.kills==80);
    assert(g.status==VICTORY);
    assert(g.score==900);
    assert(!game_active(&g));
    assert(game_valid(&g));
}

/* Testa a condição de derrota. */
static void test_game_defeat(void){
    GameState g;
    game_init(&g, 0);
    /*
     * Simula uma partida na segunda onda,
     * com apenas um ponto de vida e um inimigo
     * prestes a chegar à base.
     */
    g.wave=2;
    g.spawned=2;
    g.escaped=9;
    g.hp=1;
    g.wave_turn=1;
    g.enemies[0].hp=game_enemy_hp(2, 0);
    g.enemies[0].step=(int)g.board.path_length-2;
    assert(game_valid(&g));
    game_tick(&g);
    assert(g.hp==0);
    assert(g.escaped==10);
    assert(g.status==DEFEAT);
    assert(!game_active(&g));
    assert(game_valid(&g));
}

/* Testa a execução de um turno. */
static void test_game_tick(void){
    GameState g;
    game_init(&g, 0);
    game_tick(&g);
    assert(g.turn==1);
    assert(g.wave_turn==1);
    assert(g.spawned==1);
    assert(game_valid(&g));
    game_tick(NULL);
}

/* Testa a verificação da partida. */
static void test_game_valid(void){
    GameState g;
    GameState copy;
    game_init(&g, 0);
    assert(game_valid(&g));
    assert(!game_valid(NULL));

    /* Vida negativa é inválida. */
    copy=g;
    copy.hp=-1;
    assert(!game_valid(&copy));

    /* Dinheiro negativo é inválido. */
    copy=g;
    copy.money=-1;
    assert(!game_valid(&copy));

    /* Dificuldade inválida. */
    copy=g;
    copy.difficulty=3;
    assert(!game_valid(&copy));

    /* Status inválido. */
    copy=g;
    copy.status=(GameStatus)99;
    assert(!game_valid(&copy));

    /* Quantidade negativa de torres. */
    copy=g;
    copy.tower_count=-1;
    assert(!game_valid(&copy));

    /* Quantidade de torres acima do limite. */
    copy=g;
    copy.tower_count=MAX_TOWERS+1;
    assert(!game_valid(&copy));

    /* Onda inválida. */
    copy=g;
    copy.wave=0;
    assert(!game_valid(&copy));

    /* Quantidade de inimigos gerados negativa. */
    copy=g;
    copy.spawned=-1;
    assert(!game_valid(&copy));

    /* Pontuação inconsistente. */
    copy=g;
    copy.score=10;
    assert(!game_valid(&copy));

    /* Dinheiro inconsistente. */
    copy=g;
    copy.money++;
    assert(!game_valid(&copy));

    /* Inimigo com vida negativa. */
    copy=g;
    copy.enemies[0].hp=-1;
    assert(!game_valid(&copy));

    /* Inimigo eliminado com posição diferente de zero. */
    copy=g;
    copy.enemies[0].step=1;
    assert(!game_valid(&copy));
}

/* Testa se game_valid rejeita contadores gigantes sem estourar int. */
static void test_game_valid_overflow(void){
    GameState g;
    game_init(&g, 0);

    /* kills gigante, como num save corrompido. */
    g.kills=INT_MAX/2;
    assert(!game_valid(&g));

    /* escaped gigante. */
    game_init(&g, 0);
    g.escaped=INT_MAX/2;
    assert(!game_valid(&g));

    /* kills e escaped somados acima do total de inimigos. */
    game_init(&g, 0);
    g.kills=game_enemy_total();
    g.escaped=1;
    assert(!game_valid(&g));

    /* score gigante. */
    game_init(&g, 0);
    g.score=INT_MAX;
    assert(!game_valid(&g));
}

/* Testa se uma partida encerrada não está ativa. */
static void test_game_active(void){
    GameState g;
    game_init(&g, 0);
    assert(game_active(&g));
    g.status=DEFEAT;
    assert(!game_active(&g));
    g.status=VICTORY;
    assert(!game_active(&g));
    assert(!game_active(NULL));
}

int main(void){
    test_tower_letter();
    test_enemy_hp();
    test_wave_size();
    test_game_init();
    test_game_create();
    test_game_buy();
    test_game_combat();
    test_game_movement();
    test_game_waves();
    test_game_victory();
    test_game_defeat();
    test_game_tick();
    test_game_valid();
    test_game_valid_overflow();
    test_game_active();
    printf("Todos os testes passaram!\n");
    return 0;
}