


#include "util.hpp"


#define NONE 0
#define TIE  3
#define MAX_SCORE (1000*1000)
#define MAX_MOVES (3*3*3*3)
#define NEXT_PLAYER(player) (player ^ 3) // 0b01 <-> 0b10
#define USE_HEURISTIC_LOOKUP_TABLE
#define USE_WINNER_LOOOKUP_TABLE

struct Move {
    u8 big_move;
    u8 small_move;
};

typedef SmallVector<Move, MAX_MOVES> Moves;
typedef Array<u8, 3> ThreeMoves;
typedef Array<u8, 9> NineMoves;
typedef Array<int, 9> Weights;

Weights default_weights{
    3, 2, 3,
    2, 4, 2,
    3, 2, 3,
};

NineMoves default_move_order{4, 1, 3, 5, 7, 0, 2, 6, 8};

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

#ifdef USE_WINNER_LOOOKUP_TABLE
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

    int heuristic_player(u8 player, const Weights &weights) const {
        int score = 0;
        for (u8 move = 0; move < 9; move++){
            score += (get(move) == player)*weights[move];
        }
        return score;
    }

    int heuristic(const Weights &weights) const {
        return heuristic_player(1, weights) - heuristic_player(2, weights);
    }

    u8 update(){
        n_moves = 0;
        for (u8 move = 0; move < 9; move++){
            n_moves += get(move) != NONE;
        }

        u8 result = n_moves == 9 ? TIE : NONE;

        if (is_winner(*this, 1)) result = 1;
        if (is_winner(*this, 2)) result = 2;

        return result;
    }
};

struct MacroBoard {
    Array<MicroBoard, 9> micro_boards;
    MicroBoard winners;
    Moves moves;

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
        if (!can_play(move)){
            printf("ERROR: player %u wants to play invalid move (%u, %u)\n", player, move.big_move, move.small_move);
            print();
            assert(can_play(move));
        }

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
/*
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
*/
    static char translate(u8 player){
        assert(player < 3);
        switch (player){
        case NONE: return '-';
        case 1: return 'X';
        case 2: return 'O';
        default: return '?';
        }
    }

    void print(){
        for (int y = 0; y < 3; y++){
            for (int i = 0; i < 3; i++){
                for (int x = 0; x < 3; x++){
                    const MicroBoard &micro_board = micro_boards[x + y*3];
                    u8 a = micro_board.get(0 + i*3);
                    u8 b = micro_board.get(1 + i*3);
                    u8 c = micro_board.get(2 + i*3);
                    printf("%c%c%c", translate(a), translate(b), translate(c));
                }
                printf("\n");
            }
        }
    }
};

Move get_random_move(const MacroBoard &macro_board, u8 player){
    UNUSED(player);

    Moves moves;
    for (u8 big_move = 0; big_move < 9; big_move++){
        if (!macro_board.can_play_big_move(big_move)) continue;
        const MicroBoard &micro_board = macro_board.micro_boards[big_move];
        for (u8 small_move = 0; small_move < 9; small_move++){
            if (!micro_board.can_play(small_move)) continue;
            Move move{big_move, small_move};
            moves.push_back(move);
        }
    }

    assert(moves.size() > 0);

    return moves[rd() % moves.size()];
}

template <typename GET_MOVE_PLAYER1, typename GET_MOVE_PLAYER2>
u8 get_winner(MacroBoard &macro_board, GET_MOVE_PLAYER1 &get_move_player1, GET_MOVE_PLAYER2 &get_move_player2, u8 player, Moves &moves){
    for (int round = 0; round < MAX_MOVES; round++){

        Move move;
        if (player == 1){
            move = get_move_player1(macro_board, player);
        }else{
            move = get_move_player2(macro_board, player);
        }
        moves.push_back(move);

        u8 winner = macro_board.play(move, player);

        if (winner != NONE){
            return winner;
        }

        player = NEXT_PLAYER(player);
    }

    return TIE;
}

struct MacroScore {
    int value;
    Moves moves;
};

struct MacroAlphaBeta {
    MacroBoard macro_board;

    int lookahead;
    Weights weights;
    NineMoves move_order;

    MacroAlphaBeta(
        int lookahead,
        const Weights &weights = default_weights,
        const NineMoves &move_order = default_move_order
    ):
        lookahead(lookahead),
        weights(weights),
        move_order(move_order)
    {}

    MacroScore descend(u8 player, int depth, int alpha = -MAX_SCORE, int beta = +MAX_SCORE){
        u8 opponent = NEXT_PLAYER(player);

        if (depth == 0){
#ifdef USE_HEURISTIC_LOOKUP_TABLE
            int score = micro_board_score[macro_board.winners.fields];
#else
            int score = macro_board.winners.heuristic(weights);
#endif
            if (player == 2) score = -score;
            return MacroScore{score, macro_board.moves};
        }

        MacroScore best_score;
        bool no_score = true;

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

#if 1
        // discard all previous moves except the last one for faster copying
        if (!macro_board.moves.empty()){
            auto &moves = this->macro_board.moves;
            Move last_move = moves.back();
            moves.clear();
            moves.push_back(last_move);
        }
#endif

        MacroScore score = descend(player, lookahead);

#if 1
        assert(!score.moves.empty());
        return score.moves[macro_board.moves.empty() ? 0 : 1];
#else
        assert(score.moves.size() > macro_board.moves.size());
        return score.moves[macro_board.moves.size()];
#endif
    }
};

MacroBoard from_buffer(const Array<char, MAX_MOVES> &buffer, int x0, int y0){
    MacroBoard macro_board;
    int k = 0;
    if (x0 != -1 && y0 != -1){
        macro_board.moves.push_back(Move{66, u8(x0 + y0*3)});
    }
    for (int y = 0; y < 3; y++){
        for (int i = 0; i < 3; i++){
            for (int x = 0; x < 3; x++){
                for (int j = 0; j < 3; j++){
                    u8 move = i*3 + j;
                    char c = buffer[k++];
                    if (c == '-') continue;
                    if (c != 'X' && c != 'O'){
                        fprintf(stderr, "Invalid character: %c\n", c);
                        assert("Invalid character");
                    }
                    u8 player = c == 'X' ? 1 : 2;
                    macro_board.micro_boards[x + y*3].unsafe_set(move, player);
                }
            }
        }
    }
    for (u8 move = 0; move < 9; move++){
        u8 player = macro_board.micro_boards[move].update();
        if (player == NONE) continue;
        macro_board.winners.play(move, player);
    }
    return macro_board;
}

void init(){
    MicroBoard micro_board;
    for (u32 i = 0; i < (1 << 18); i++){
        micro_board.fields = i;
#ifdef USE_WINNER_LOOOKUP_TABLE
        micro_board_winner[i] = micro_board.update();
#endif

#ifdef USE_HEURISTIC_LOOKUP_TABLE
        micro_board_score[i] = micro_board.heuristic(default_weights);
#endif
    }
}

Move get_probable_move(const MacroBoard &macro_board0, u8 player){
    Array<Array<int, 9>, 9> scores = {};
    Array<Array<int, 9>, 9> counts = {};

    for (int i = 0; i < 100*1000; i++){
        MacroBoard macro_board = macro_board0;
        Moves moves;
        u8 winner = get_winner(macro_board, get_random_move, get_random_move, player, moves);
        Move first_move = moves[0];
        counts[first_move.big_move][first_move.small_move]++;
        if (winner == player){
            scores[first_move.big_move][first_move.small_move] += 1;
        }
        if (winner != player){
            scores[first_move.big_move][first_move.small_move] -= 1;
        }
    }

    int max_score = INT_MIN;
    bool no_move = true;
    Move move;

    for (u8 big_move = 0; big_move < 9; big_move++){
        for (u8 small_move = 0; small_move < 9; small_move++){
            if (counts[big_move][small_move] == 0) continue;
            int score = scores[big_move][small_move];
            if (no_move || max_score < score){
                max_score = score;
                no_move = false;
                move = Move{big_move, small_move};
            }
        }
    }

    return move;
}

int main(){
    init();

    for (int i = 0; i < 10; i++){
        Moves moves;
        MacroBoard macro_board;
        MacroAlphaBeta get_smart_move(6);
        // lookahead 6, probable move wins 8/10 on first turn and 7/10 on second turn
        u8 winner = get_winner(macro_board, get_probable_move, get_smart_move, 1, moves);
        printf("winner: %u\n", winner);
    }
#if 0
    {

        char player_symbol;
        int x, y;
        int ret = scanf(" %c %i %i", &player_symbol, &y, &x);
        if (ret != 3){
            fprintf(stderr, "Expected board instructions\n");
            exit(-1);
        }
        assert(player_symbol == 'X' || player_symbol == 'O');

        Array<char, MAX_MOVES> buffer;
        for (int i = 0; i < MAX_MOVES; i++){
            //printf("before reading char %i\n", i);
            int ret = scanf(" %c", &buffer[i]);
            assert(ret == 1);
            //printf("read char %i\n", i);
        }

        u8 player = player_symbol == 'X' ? 1 : 2;

        MacroBoard macro_board = from_buffer(buffer, x, y);

        MacroAlphaBeta get_smart_move(10, default_weights, default_move_order);

        Move move = get_smart_move(macro_board, player);

        int x_big, y_big, x_small, y_small;

        x_big = move.big_move % 3;
        y_big = move.big_move / 3;
        x_small = move.small_move % 3;
        y_small = move.small_move / 3;
        printf("%i %i %i %i\n", y_big, x_big, y_small, x_small);

/*
        u8 next_player = NEXT_PLAYER(player);

        puts(next_player == 1 ? "X" : "O");

        x = move.small_move % 3;
        y = move.small_move / 3;
        printf("%u %u\n", y, x);

        macro_board.play(move, player);

        macro_board.print();
        */
    }
#endif
    return 0;
}

