#include "magic.c"

int main(){
    static const uint8_t winning_board_indices[8*3] = {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,

        0, 3, 6,
        1, 4, 7,
        2, 5, 8,

        0, 4, 8,
        2, 4, 6,
    };

    assert(!HAS_WON(0));
    for (uint32_t i = 0; i < sizeof(winning_board_indices); i++){
        for (uint32_t j = 0; j < sizeof(winning_board_indices); j++){
            uint8_t a = winning_board_indices[i];
            uint8_t b = winning_board_indices[j];
            uint32_t board = move_masks[a] | move_masks[b];
            assert(!HAS_WON(board));
        }
    }
    for (uint32_t i = 0; i < sizeof(winning_board_indices); i += 3){
        uint8_t a = winning_board_indices[i + 0];
        uint8_t b = winning_board_indices[i + 1];
        uint8_t c = winning_board_indices[i + 2];
        uint32_t board = move_masks[a] | move_masks[b] | move_masks[c];
        assert(HAS_WON(board));
    }
    assert(!HAS_WON(move_masks[0] | move_masks[1] | move_masks[5] | move_masks[6] | move_masks[7]));

    const char *c =
        "X-----XXO\n"
        "-X----OOX\n"
        "--X---XXO\n"
        "---------\n"
        "---------\n"
        "---------\n"
        "------O--\n"
        "------O--\n"
        "------O--\n";
    struct board board[1];
    board_init2(board, c, INVALID_MOVE, 1);
    board_print(board);
    play(board, 0.25);

    return 0;
}
