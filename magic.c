#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

typedef uint8_t u8;
typedef uint32_t u32;
/*
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

u32 has_won(u32 fields){
    return ((fields & 7190235) + ((fields & 9586980) >> 2)) & 9586980;
}

struct board {
    u32 fields;
    u32 player;
    u32 moves;
};

void play(struct board *board, u8 move, u8 player){
    static const u32 move_masks[9] = {262657, 4098, 2129924, 1032, 4726800, 65568, 8390720, 16512, 1179904};
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

int main(){
/*
    struct board board[1];
    ZERO(board);

    play(board, 0, 1);
    play(board, 1, 1);
    play(board, 2, 1);
*/


    //printf("%i\n", 31 - __builtin_clz(x));


    return 0;
}
