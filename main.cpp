#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <array>
#include <vector>
#include <algorithm>
#include <limits>

#define INF std::numeric_limits<double>::infinity()
#define NONE 0xff
#define UNUSED(x) ((void)x)

typedef uint8_t u8;
typedef uint32_t u32;

u32 rd(){
    static u32 x = 0x12345678;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

u32 rd(u32 n){
    return rd() % n;
}

typedef std::array<u8, 3> ThreeMoves;

constexpr std::array<ThreeMoves, 3 + 3 + 2> winning_positions = {
    ThreeMoves{0, 1, 2},
    ThreeMoves{3, 4, 5},
    ThreeMoves{6, 7, 8},

    ThreeMoves{0, 3, 6},
    ThreeMoves{1, 4, 7},
    ThreeMoves{2, 5, 8},

    ThreeMoves{0, 4, 8},
    ThreeMoves{2, 4, 6},
};

struct Cell {
    u8 winner;

    Cell(): winner(0){}
};

template <typename CELL>
struct Board : Cell {
    std::array<CELL, 9> cells;

    const CELL& operator [] (u8 i) const {
        assert(i < 9);
        return cells[i];
    }

    CELL& operator [] (u8 i){
        assert(i < 9);
        return cells[i];
    }

    u32 has_won(u8 player, ThreeMoves moves) const {
        for (u8 move : moves){
            if (cells[move].winner != player){
                return false;
            }
        }
        return true;
    }

    bool has_won(u8 player) const {
        for (ThreeMoves moves : winning_positions){
            if (has_won(player, moves)) return true;
        }
        return false;
    }

    void get_moves(std::vector<u8> &moves) const {
        if (has_won(1) || has_won(2)) return;
        for (u8 move = 0; move < 9; move++){
            if (cells[move].winner == 0) moves.push_back(move);
        }
    }

    bool has_moves(){
        std::vector<u8> moves;
        get_moves(moves);
        return !moves.empty();
    }
};

typedef Board<Cell> SmallBoard;
typedef Board<SmallBoard> BigBoard;

u8 get_random(std::vector<u8> &values){
    size_t index = rd(values.size());
    return values[index];
}

void print(const BigBoard &big_board){
    for (int y = 0; y < 3; y++){
        for (int i = 0; i < 3; i++){
            printf("| ");
            for (int x = 0; x < 3; x++){
                const SmallBoard &board = big_board[x + y*3];
                printf("%i %i %i | ", board[0 + i*3].winner, board[1 + i*3].winner, board[2 + i*3].winner);
            }
            printf("\n");
        }


        for (int i = 0; i < 3; i++) printf("+-------");
        printf("+\n");
    }
    printf("\n");
}

struct Node {
    SmallBoard small_board;

    double heuristic() const {
        switch (small_board.winner){
        case 1: return +INF;
        case 2: return -INF;
        default: return 0;
        }
    }

    bool is_terminal() const {
        std::vector<u8> moves;
        small_board.get_moves(moves);
        return moves.empty();
    }

    std::vector<Node> get_children(u8 player){
        std::vector<u8> moves;
        small_board.get_moves(moves);

        std::vector<Node> children;

        for (u8 move : moves){
            Node child;
            child.small_board = small_board;
            child.small_board[move].winner = player;
            if (child.small_board.has_won(player)){
                child.small_board.winner = player;
            }
            children.push_back(child);
        }

        return children;
    }
};

double alphabeta(Node node, int depth, double alpha, double beta, u8 player){
    if (depth == 0 || node.is_terminal()){
        return node.heuristic();
    }

    u8 next_player = player == 1 ? 2 : 1;

    if (player == 1){
        double value = -INF;
        for (Node child : node.get_children(player)){
            double new_value = alphabeta(child, depth - 1, alpha, beta, next_player);
            if (value < new_value){
                value = new_value;
            }
            alpha = std::max(alpha, value);
            if (beta <= alpha) break;
        }
        return value;
    }else{
        double value = +INF;
        for (Node child : node.get_children(player)){
            double new_value = alphabeta(child, depth - 1, alpha, beta, next_player);
            if (value > new_value){
                value = new_value;
            }
            beta = std::min(beta, value);
            if (beta <= alpha) break;
        }
        return value;
    }
}

struct Move {
    u8 big_move;
    u8 small_move;
};

Move get_random_move(const BigBoard &big_board, u8 big_move, u8 player){
    // random move does not depend on player
    UNUSED(player);

    std::vector<u8> non_finished_small_boards;
    std::array<std::vector<u8>, 9> available_fields;

    // get all usable empty fields
    for (u8 i = 0; i < 9; i++){
        big_board[i].get_moves(available_fields[i]);
        if (!available_fields[i].empty()){
            non_finished_small_boards.push_back(i);
        }
    }

    // if big board is not forced, choose one at random
    if (big_move == NONE){
        big_move = get_random(non_finished_small_boards);
        printf("Error: no big board selected, choose %u at random\n", big_move);
    }

    if (available_fields[big_move].empty()){
        printf("no more empty fields available in big board %u\n", big_move);
        exit(-1);
    }

    // choose move in small board at random
    u8 small_board_move = get_random(available_fields[big_move]);

    return Move{big_move, small_board_move};
}

void play(){
    BigBoard big_board;

    auto get_player1_move = get_random_move;
    auto get_player2_move = get_random_move;

    u8 player = 1;
    u8 big_move = NONE;

    for (u32 round = 0; round < 3*3*3*3; round++){
        printf("round %u\n", round);

        auto get_move = (player == 1) ? get_player1_move : get_player2_move;

        Move move = get_move(big_board, big_move, player);

        printf("player %u places at %u %u\n", player, move.big_move, move.small_move);

        assert(big_move == NONE || move.big_move == big_move);

        SmallBoard &small_board = big_board[move.big_move];

        assert(small_board.has_moves());

        Cell &cell = small_board[move.small_move];

        assert(cell.winner == 0);

        cell.winner = player;

        print(big_board);

        if (small_board.has_won(player)){
            small_board.winner = player;

            big_move = NONE;

            if (big_board.has_won(player)){
                big_board.winner = player;

                printf("player %u wins\n", player);

                break;
            }
        }

        big_move = move.small_move;

        if (!big_board[big_move].has_moves()){
            big_move = -1;
        }

        player = (player == 1) ? 2 : 1;
    }
}

int main(){

    play();
/*
    Timer timer;

    Node root;
    double result = alphabeta(root, 10, -INF, +INF, 1);
    printf("%f\n", timer.stop());
    printf("result: %f\n", result);
    printf("\n");

    root.small_board[0].winner = 1;
    for (int depth = 0; depth < 10; depth++) printf("%i: %f\n", depth, alphabeta(root, depth, -INF, +INF, 2));
    printf("\n");
    root.small_board[2].winner = 2;
    for (int depth = 0; depth < 10; depth++) printf("%i: %f\n", depth, alphabeta(root, depth, -INF, +INF, 1));
*/
    return 0;
}
