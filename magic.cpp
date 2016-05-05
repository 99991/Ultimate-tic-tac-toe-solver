#include "util.hpp"
#include "timer.hpp"

#if 0
u32 u32_bin(const char *c){
    u32 x = 0;
    for (; *c; c++){
        if (*c == '0') x <<= 1;
        if (*c == '1') x = (x << 1) | 1;
    }
    return x;
}

void print_bin(u32 x, int split){
    for (int i = 31; i >= 0; i--){
        putchar((x >> i) & 1 ? '1' : '0');
        if (i > 0 && i % split == 0) putchar('_');
    }
    putchar('\n');
}

static const u8 winning_lines[8*3] = {
    0, 1, 2,
    3, 4, 5,
    6, 7, 8,

    0, 3, 6,
    1, 4, 7,
    2, 5, 8,

    0, 4, 8,
    2, 4, 6,
};

u32 repeat(u32 pattern, u32 n, u32 stride){
    u32 x = 0;
    for (u32 i = 0; i < n; i++) x = (x << stride) | pattern;
    return x;
}

u32 move_mask(u8 move){
    u32 result = 0;
    for (u32 i = 0; i < sizeof(winning_lines); i++){
        if (winning_lines[i] == move){
            result |= 1 << i;
        }
    }
    return result;
}
*/


struct board {
    u32 fields;
    u32 player;
    u32 moves;
};

void play(struct board *board, u8 move, u8 player){
    u8 mask = move_masks[move];
    board->fields |= mask;
    board->player |= player == 1 ? mask : 0;
}

#define ZERO(ptr) memset(ptr, 0, sizeof(*ptr))

void print_moves(u32 x){
    while (x){
        u8 i = 31 - __builtin_clz(x);
        printf("%u\n", i);
        x ^= (u32)1 << i;
    }
}
#endif
/*
struct hilo {
    u32 hi, lo;
};

u64 diff(struct hilo *a, struct hilo *b){
    u64 t0 = (((u64)a->hi) << 32) | a->lo;
    u64 t1 = (((u64)b->hi) << 32) | b->lo;
    return t1 - t0;
}

#define TICKS(t) asm("rdtscp" : "=a"(t.lo), "=d"(t.hi))
*/

u32 has_won(u32 fields){
    return ((fields & 7190235) + ((fields & 9586980) >> 2)) & 9586980;
}

u32 move_masks[9] = {262657, 4098, 2129924, 1032, 4726800, 65568, 8390720, 16512, 1179904};

Array<SmallVector<u8, 9>, (1 << 9)> index_table;

u8 play(SmallVector<u8, 9> &moves){
    u32 player1_fields = 0;
    u32 player2_fields = 0;
    u32 available_moves = 0x1ff;
    u8 player = 1;

    while (available_moves){
    //while (moves.size() < 9){
        const auto &indices = index_table[available_moves];
        u32 move = indices[rd() % indices.size()];
        moves.push_back(move);
        u32 mask = move_masks[move];

        if (player == 1){
            player1_fields |= mask;
            if (has_won(player1_fields)){
                return 1;
            }
        }else{
            player2_fields |= mask;
            if (has_won(player2_fields)){
                return 2;
            }
        }

        available_moves &= ~(1 << move);
        player ^= 3;
    }

    return 0;
}

int main(){
    for (u32 mask = 0; mask < (1 << 9); mask++){
        for (int i = 0; i < 9; i++){
            if ((mask >> i) & 1){
                index_table[mask].push_back(i);
            }
        }
    }

    Array<Array<int, 3>, 9> counts = {};
    for (u32 k = 0; k < 1000*1000; k++){
        SmallVector<u8, 9> moves;
        u8 winner = play(moves);
        counts[moves[0]][winner]++;
    }

    for (int j = 0; j < 9; j++){
        printf("%i: ", j);
        for (int i = 0; i < 3; i++){
            printf("%i ", counts[j][i]);
        }
        printf("\n");
    }
/*
    Array<int, 9> counts = {};

    for (int k = 0; k < 100; k++){

        Timer timer;

        for (u32 i = 0; i < 9*1000*1000; i++){
            const auto &indices = index_table[rd() % (index_table.size() - 1) + 1];
            u32 index = indices[rd() % indices.size()];
            //u8 index = choose_randomly(indices);
            counts[index]++;
        }

        printf("%f ms\n", timer.stop()*1000);
    }

    for (int x : counts){
        printf("%i\n", x);
    }
*/
    return 0;
}
