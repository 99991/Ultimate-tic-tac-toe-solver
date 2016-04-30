#include "common.hpp"

#define NONE 0
#define TIE  3

#define MAX_SCORE (1000*1000)

struct Move {
    u8 big_move;
    u8 small_move;
};

typedef Array<u8, 3> ThreeMoves;

constexpr Array<ThreeMoves, 3 + 3 + 2> wins = {
    ThreeMoves{0, 1, 2}, // 0
    ThreeMoves{3, 4, 5}, // 1
    ThreeMoves{6, 7, 8}, // 2

    ThreeMoves{0, 3, 6}, // 3
    ThreeMoves{1, 4, 7}, // 4
    ThreeMoves{2, 5, 8}, // 5

    ThreeMoves{0, 4, 8}, // 6
    ThreeMoves{2, 4, 6}, // 7
};

u8 micro_board_winner[1 << 18];
int micro_board_score[1 << 18];

template <typename BOARD>
bool is_winner(const BOARD &board, u8 player, ThreeMoves moves){
    for (u8 move : moves){
        if (board.get(move) != player){
            return false;
        }
    }
    return true;
}

template <typename BOARD>
bool is_winner(const BOARD &board, u8 player){
    for (ThreeMoves moves : wins){
        if (is_winner(board, player, moves)){
            return true;
        }
    }
    return false;
}

struct MicroBoard {
    u32 fields;
    u8 n_moves;

    MicroBoard(): fields(0), n_moves(9){
        for (u8 move = 0; move < 9; move++){
            unsafe_set(move, NONE);
        }
    }

    void unsafe_set(u8 move, u8 player){
        fields |= player << move*2;
    }

    u8 get(u8 move) const {
        return (fields >> move*2) & 3;
    }

    void clr(u8 move){
        assert(get(move) != NONE);
        n_moves++;
        fields &= ~(u32(3) << move*2);
    }

    bool can_play(u8 move) const {
        return get(move) == NONE;
    }

    u8 play(u8 move, u8 player){
        assert(can_play(move));
        n_moves--;

        unsafe_set(move, player);

#if 1
        if (player == TIE) return n_moves == 0 ? TIE : NONE;

        return micro_board_winner[fields];
#else
        if (player != TIE && ::is_winner(*this, player)){
            return player;
        }

        return n_moves == 0 ? TIE : NONE;
#endif
    }

    void print(){
        printf("+-------+\n");
        for (int y = 0; y < 3; y++){
            printf("| ");
            for (int x = 0; x < 3; x++){
                printf("%i ", get(x + y*3));
            }
            printf("|\n");
        }
        printf("+-------+\n\n");
    }

    int get_heuristic_score(u8 player) const {
        int score = 0;
        for (u8 move = 0; move < 9; move++){
            score += (get(move) == player);
        }
        return score;
    }

    int heuristic() const {
        return get_heuristic_score(1) - get_heuristic_score(2);
    }
};

struct MacroBoard {
    Array<MicroBoard, 9> micro_boards;
    MicroBoard winners;
    SmallVector<Move, 3*3*3*3> moves;

    bool can_play_anywhere() const {
        // if no moves done yet
        if (moves.empty()) return true;

        // if forced small board is decided already
        return !winners.can_play(moves.back().small_move);
    }

    bool can_play_big_move(u8 big_move) const {
        // if small board is decided already, it can't be played
        if (!winners.can_play(big_move)) return false;

        if (can_play_anywhere()) return true;

        return moves.back().small_move == big_move;
    }

    bool can_play(Move move) const {

        if (!can_play_big_move(move.big_move)) return false;

        // small field to play must still be empty
        if (!micro_boards[move.big_move].can_play(move.small_move)) return false;

        return true;
    }

    void undo(){
        assert(!moves.empty());
        Move move = moves.back();
        if (winners.get(move.big_move) != NONE){
            winners.clr(move.big_move);
        }
        micro_boards[move.big_move].clr(move.small_move);
        moves.pop_back();
    }

    u8 play(Move move, u8 player){
        assert(can_play(move));

        moves.push_back(move);

        u8 winner = micro_boards[move.big_move].play(move.small_move, player);
        if (winner != NONE){
            winner = winners.play(move.big_move, winner);
            if (winner != NONE){
                return winner;
            }
        }

        return NONE;
    }

    void print(){
        for (int i = 0; i < 3; i++) printf("+-------");
        printf("+\n");

        for (int y = 0; y < 3; y++){
            for (int i = 0; i < 3; i++){
                printf("| ");
                for (int x = 0; x < 3; x++){
                    const MicroBoard &micro_board = micro_boards[x + y*3];
                    u8 a = micro_board.get(0 + i*3);
                    u8 b = micro_board.get(1 + i*3);
                    u8 c = micro_board.get(2 + i*3);
                    printf("%i %i %i | ", a, b, c);
                }
                printf("\n");
            }

            for (int i = 0; i < 3; i++) printf("+-------");
            printf("+\n");
        }
        winners.print();
        printf("\n\n");
    }
};

Move get_random_move(const MacroBoard &macro_board, u8 player){
    UNUSED(player);

    SmallVector<Move, 3*3*3*3> moves;
    for (u8 big_move = 0; big_move < 9; big_move++){
        if (!macro_board.can_play_big_move(big_move)) continue;
        const MicroBoard &micro_board = macro_board.micro_boards[big_move];
        for (u8 small_move = 0; small_move < 9; small_move++){
            if (!micro_board.can_play(small_move)) continue;
            Move move{big_move, small_move};
            moves.push_back(move);
        }
    }

    assert(!moves.empty());

    return moves[rd() % moves.size()];
}

template <typename GET_MOVE_PLAYER1, typename GET_MOVE_PLAYER2>
u8 get_winner(GET_MOVE_PLAYER1 &get_move_player1, GET_MOVE_PLAYER2 &get_move_player2){
    MacroBoard macro_board;

    u8 player = 1;

    for (int round = 0; round < 3*3*3*3; round++){

        Move move;
        if (player == 1){
            move = get_move_player1(macro_board, player);
        }else{
            move = get_move_player2(macro_board, player);
        }

        u8 winner = macro_board.play(move, player);

        if (winner != NONE){
            return winner;
        }

        player = (player == 1) ? 2 : 1;
    }

    return TIE;
}

struct MacroScore {
    int value;
    SmallVector<Move, 81> moves;
};

struct MacroAlphaBeta {
    MacroBoard macro_board;

    int lookahead;

    MacroAlphaBeta(int lookahead): lookahead(lookahead){}

    MacroScore descend(u8 player, int depth, int alpha = -MAX_SCORE, int beta = +MAX_SCORE){
        u8 opponent = player ^ 3;

        if (depth == 0){
#if 0
            int score = macro_board.winners.heuristic();
#else
            int score = micro_board_score[macro_board.winners.fields];
#endif
            if (player == 2) score = -score;
            return MacroScore{score, macro_board.moves};
        }

        MacroScore best_score;
        bool no_score = true;
        best_score.value = -MAX_SCORE;

        constexpr static Array<u8, 9> move_order{4, 0, 2, 6, 8, 1, 3, 5, 7};
        for (u8 big_move : move_order){
            if (!macro_board.can_play_big_move(big_move)) continue;
            const MicroBoard &micro_board = macro_board.micro_boards[big_move];
            for (u8 small_move : move_order){
                if (!micro_board.can_play(small_move)) continue;
                Move move{big_move, small_move};

                u8 winner = macro_board.play(move, player);

                MacroScore new_score;

                if (winner == NONE){
                    new_score = descend(opponent, depth - 1, -beta, -alpha);
                    new_score.value = -new_score.value;
                }else if (winner == TIE){
                    new_score = MacroScore{0, macro_board.moves};
                }else{
                    int score = (player == winner) ? +MAX_SCORE : -MAX_SCORE;
                    new_score = MacroScore{score, macro_board.moves};
                }

                macro_board.undo();

                if (no_score || best_score.value < new_score.value){
                    no_score = false;
                    best_score = new_score;
                }
                if (alpha < best_score.value) alpha = best_score.value;
                if (beta <= alpha) return best_score;
            }
        }

        // there must have been a living branch because game did not tie yet
        assert(!no_score);

        return best_score;
    }

    Move operator () (const MacroBoard &macro_board, u8 player){
        this->macro_board = macro_board;

        MacroScore score = descend(player, lookahead);

        return score.moves[macro_board.moves.size()];
    }
};

#include "timer.hpp"

void test();

int main(){
#if 1
    MicroBoard micro_board;
    for (u32 i = 0; i < (1 << 18); i++){
        micro_board.fields = i;
        u32 n_set = 0;
        for (u8 move = 0; move < 9; move++){
            n_set += micro_board.get(move) != NONE;
        }
        u32 result = n_set == 9 ? TIE : NONE;
        if (is_winner(micro_board, 1)) result = 1;
        if (is_winner(micro_board, 2)) result = 2;

        micro_board_winner[i] = result;
        micro_board_score[i] = micro_board.heuristic();
    }
    printf("small board winners calculated\n");
#endif
    test();

    Timer timer;
    int winners[4] = {0, 0, 0, 0};
    for (int i = 0; i < 100; i++){
        MacroAlphaBeta opponent(3);
        u8 winner = get_winner(get_random_move, opponent);
        winners[winner]++;
    }
    printf("dt: %f seconds\n", timer.stop());
    for (int i = 0; i < 4; i++){
        printf("%i: %i\n", i, winners[i]);
    }

    return 0;
}

void test(){
    // Test tie small board
    MicroBoard micro_board;
    assert(NONE == micro_board.play(0, 1));
    assert(NONE == micro_board.play(1, 1));
    assert(NONE == micro_board.play(2, 2));
    assert(NONE == micro_board.play(3, 2));
    assert(NONE == micro_board.play(4, 2));
    assert(NONE == micro_board.play(5, 1));
    assert(NONE == micro_board.play(6, 1));
    assert(NONE == micro_board.play(7, 1));
    assert(TIE  == micro_board.play(8, 2));

    // Test win small board player 1
    micro_board = MicroBoard();
    assert(NONE == micro_board.play(0, 1));
    assert(NONE == micro_board.play(1, 1));
    assert(1    == micro_board.play(2, 1));

    // Test win small board player 2
    micro_board = MicroBoard();
    assert(NONE == micro_board.play(0, 2));
    assert(NONE == micro_board.play(1, 2));
    assert(2    == micro_board.play(2, 2));

    // Test win big board player 1
    MacroBoard macro_board;
    assert(NONE == macro_board.play(Move{0, 1}, 1));
    assert(NONE == macro_board.play(Move{1, 2}, 1));
    assert(NONE == macro_board.play(Move{2, 0}, 1));
    //assert(macro_board.next_big_move == 0);
    assert(NONE == macro_board.play(Move{0, 2}, 1));
    assert(NONE == macro_board.play(Move{2, 1}, 1));
    assert(NONE == macro_board.play(Move{1, 0}, 1));

    assert(NONE == macro_board.play(Move{0, 0}, 1));
    assert(macro_board.winners.get(0) == 1);
    assert(!macro_board.can_play(Move{0, 3}));

    assert(NONE == macro_board.play(Move{1, 1}, 1));
    assert(macro_board.winners.get(1) == 1);
    assert(!macro_board.can_play(Move{1, 4}));

    assert(1 == macro_board.play(Move{2, 2}, 1));
    assert(macro_board.winners.get(2) == 1);
    assert(!macro_board.can_play(Move{2, 5}));
}
