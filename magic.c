#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static uint32_t rd(){
    static uint32_t x = 0x12345678;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

#define GET_BIT32(x, i) (((x) >> (i)) & 1)
#define CLEAR_BIT32(x, i) x &= ~(((uint32_t)1) << i)

#define MAX_MOVES (3*3*3*3)
#define INVALID_MOVE 0xff
#define ALL_MOVES ((1 << 9) - 1)
#define HAS_WON(fields) (((fields) + 0x11111111) & 0x88888888)

static const uint32_t move_masks[9] = {
    1074004032, 536887296, 268436484,
    67239936, 33562658, 16777728,
    4259841, 2101248, 1048848,
};

struct moves {
    uint8_t macro_moves[MAX_MOVES];
    uint8_t micro_moves[MAX_MOVES];
    uint8_t n;
};

static void moves_add(struct moves *a, uint8_t macro_move, uint8_t micro_move){
    assert(a->n < MAX_MOVES);
    a->macro_moves[a->n] = macro_move;
    a->micro_moves[a->n] = micro_move;
    a->n++;
}

static uint8_t get_random_move(uint32_t moves){
    while (1){
        uint8_t i = rd() % 9;
        if (GET_BIT32(moves, i)) return i;
    }
}

struct board {
    uint8_t player;
    uint8_t forced_macro_move;

    uint32_t nine_micro_moves[9];
    uint32_t nine_player1_micro_boards[9];
    uint32_t nine_player2_micro_boards[9];

    uint32_t macro_moves;
    uint32_t player1_macro_board;
    uint32_t player2_macro_board;
};
/*
static void board_init(struct board *board){
    for (int i = 0; i < 9; i++){
        board->nine_micro_moves[i] = ALL_MOVES;
        board->nine_player1_micro_boards[i] = 0;
        board->nine_player2_micro_boards[i] = 0;
    }

    board->macro_moves = ALL_MOVES;
    board->player1_macro_board = 0;
    board->player2_macro_board = 0;

    board->player1_turn = 1;
    board->forced_macro_move = INVALID_MOVE;
}
*/

void board_print(struct board *board){
    printf("+-----+-----+-----+\n");
    for (int macro_y = 0; macro_y < 3; macro_y++){
        for (int micro_y = 0; micro_y < 3; micro_y++){
            printf("| ");
            for (int macro_x = 0; macro_x < 3; macro_x++){
                int macro_move = macro_x + macro_y*3;
                for (int micro_x = 0; micro_x < 3; micro_x++){
                    int micro_move = micro_x + micro_y*3;
                    uint32_t mask = move_masks[micro_move];
                    if ((board->nine_player1_micro_boards[macro_move] & mask) == mask){
                        printf("X");
                    }else if ((board->nine_player2_micro_boards[macro_move] & mask) == mask){
                        printf("O");
                    }else{
                        printf("-");
                    }
                }
                printf(" | ");
            }
            printf("\n");
        }
        printf("+-----+-----+-----+\n");
    }
    printf("\n");
    printf("+-----+\n");
    for (int y = 0; y < 3; y++){
        printf("| ");
        for (int x = 0; x < 3; x++){
            int i = x + y*3;
            uint32_t mask = move_masks[i];
            if ((board->player1_macro_board & mask) == mask){
                printf("X");
            }else if ((board->player2_macro_board & mask) == mask){
                printf("O");
            }else if (GET_BIT32(board->macro_moves, i)){
                printf("-");
            }else{
                printf("T");
            }
        }
        printf(" |\n");
    }
    printf("+-----+\n\n");
}

static void board_init2(struct board *board, const char *c, uint8_t macro_move, uint8_t player){
    for (int i = 0; i < 9; i++){
        board->nine_micro_moves[i] = ALL_MOVES;
        board->nine_player1_micro_boards[i] = 0;
        board->nine_player2_micro_boards[i] = 0;
    }

    for (int macro_y = 0; macro_y < 3; macro_y++){
        for (int micro_y = 0; micro_y < 3; micro_y++){
            for (int macro_x = 0; macro_x < 3; macro_x++){
                int macro_move = macro_x + macro_y*3;
                for (int micro_x = 0; micro_x < 3; micro_x++){
                    int micro_move = micro_x + micro_y*3;
                    while (isspace(*c)) c++;
                    assert(*c == '-' || *c == 'X' || *c == 'O');
                    if (*c != '-'){
                        CLEAR_BIT32(board->nine_micro_moves[macro_move], micro_move);
                        if (*c == 'X'){
                            board->nine_player1_micro_boards[macro_move] |= move_masks[micro_move];
                        }else{
                            board->nine_player2_micro_boards[macro_move] |= move_masks[micro_move];
                        }
                    }
                    c++;
                }
            }
        }
    }

    board->macro_moves = ALL_MOVES;
    board->player1_macro_board = 0;
    board->player2_macro_board = 0;

    for (int i = 0; i < 9; i++){
        if (HAS_WON(board->nine_player1_micro_boards[i])){
            CLEAR_BIT32(board->macro_moves, i);
            board->player1_macro_board |= move_masks[i];
        }else if (HAS_WON(board->nine_player2_micro_boards[i])){
            CLEAR_BIT32(board->macro_moves, i);
            board->player2_macro_board |= move_masks[i];
        }else if (!board->nine_micro_moves[i]){
            CLEAR_BIT32(board->macro_moves, i);
        }
    }

    board->player = player;
    board->forced_macro_move = macro_move;
}

static uint8_t make_move(struct board *board, struct moves *moves, uint8_t macro_move, uint8_t micro_move){
    /* check if macro move is legal */
    assert(GET_BIT32(board->macro_moves, macro_move) == 1);

    /* if forced macro move is legal, it should be taken */
    if (board->forced_macro_move != INVALID_MOVE && GET_BIT32(board->macro_moves, board->forced_macro_move)){
        assert(board->forced_macro_move == macro_move);
    }

    uint32_t *micro_moves = &board->nine_micro_moves[macro_move];

    /* disallow micro move in the future */
    CLEAR_BIT32(*micro_moves, micro_move);

    /* force enemy to play in macro board that is current micro board */
    board->forced_macro_move = micro_move;

    /* record move */
    moves_add(moves, macro_move, micro_move);

    if (board->player == 1){
        /* play on micro board */
        uint32_t *micro_board = &board->nine_player1_micro_boards[macro_move];
        *micro_board |= move_masks[micro_move];

        /* if micro board has been won */
        if (HAS_WON(*micro_board)){
            /* play on macro board */
            board->player1_macro_board |= move_masks[macro_move];

            /* disallow macro board in the future */
            CLEAR_BIT32(board->macro_moves, macro_move);

            if (HAS_WON(board->player1_macro_board)){
                return 1;
            }
        }
    }else{
        /* play on micro board */
        uint32_t *micro_board = &board->nine_player2_micro_boards[macro_move];
        *micro_board |= move_masks[micro_move];

        /* if micro board has been won */
        if (HAS_WON(*micro_board)){
            /* play on macro board */
            board->player2_macro_board |= move_masks[macro_move];

            /* disallow macro board in the future */
            CLEAR_BIT32(board->macro_moves, macro_move);

            if (HAS_WON(board->player2_macro_board)){
                return 2;
            }
        }
    }

    /* if micro board is full */
    if (!*micro_moves){
        /* disallow macro board in the future */
        CLEAR_BIT32(board->macro_moves, macro_move);
    }

    board->player ^= 3;

    return 0;
}

static uint8_t make_random_move(struct board *board, struct moves *moves){
    /* choose macro board */
    uint8_t macro_move = board->forced_macro_move;
    if (macro_move == INVALID_MOVE || GET_BIT32(board->macro_moves, macro_move) == 0){
        macro_move = get_random_move(board->macro_moves);
    }

    /* choose micro board */
    uint8_t micro_move = get_random_move(board->nine_micro_moves[macro_move]);

    return make_move(board, moves, macro_move, micro_move);
}

static uint8_t play_ultimate_tictactoe(const struct board *initial_board, struct moves *moves){
    struct board board[1] = {*initial_board};

    /* while there are macro boards that can be played in */
    while (board->macro_moves){
        uint8_t winner = make_random_move(board, moves);
        if (winner) return winner;
    }

    return 0;
}

#include <time.h>

static void play(const struct board *initial_board, double max_sec){
    int counts[9][9][3];
    memset(counts, 0, sizeof(counts));
    int n_games = 0;

    clock_t t = clock();
    struct moves moves[1];

    while (1){
        int n = 10*1000;
        for (int k = 0; k < n; k++){
            moves->n = 0;
            uint8_t winner = play_ultimate_tictactoe(initial_board, moves);

            assert(moves->n > 0);
            uint8_t macro_move = moves->macro_moves[0];
            uint8_t micro_move = moves->micro_moves[0];

            counts[macro_move][micro_move][winner]++;
        }
        n_games += n;
        double dt = (clock() - t)/(double)CLOCKS_PER_SEC;
        if (dt > max_sec){
            printf("%f million games per second\n", n_games*1e-6/dt);

            printf("+-------------------+-------------------+-------------------+\n");
            for (int macro_y = 0; macro_y < 3; macro_y++){
                for (int micro_y = 0; micro_y < 3; micro_y++){
                    printf("| ");
                    for (int macro_x = 0; macro_x < 3; macro_x++){
                        int macro_move = macro_x + macro_y*3;
                        for (int micro_x = 0; micro_x < 3; micro_x++){
                            int micro_move = micro_x + micro_y*3;
                            uint8_t player = initial_board->player;
                            int wins = counts[macro_move][micro_move][player];
                            int losses = counts[macro_move][micro_move][player ^ 3];
                            int total = wins + losses;
                            if (total == 0){
                                printf("----- ");
                            }else{
                                printf("%.3f ", wins / (double)total);
                            }
                        }
                        printf("| ");
                    }
                    printf("\n");
                }
                printf("+-------------------+-------------------+-------------------+\n");
            }
            printf("\n");

            break;
        }
    }
}

int main(){
    const char *c =
        "--- --- ---\n"
        "--- --- ---\n"
        "--- --- ---\n"

        "--- --- ---\n"
        "--- -X- ---\n"
        "--- --- ---\n"

        "--- --- ---\n"
        "--- --- ---\n"
        "--- --- ---\n";

    struct board board[1];
    board_init2(board, c, 1, 1);
    board_print(board);
    play(board, 1.0);

    return 0;
}
