#ifndef _BOARD
#define _BOARD

#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "time.h"

#define BOARD_SIZE 15
#define STONES 225
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define DRAW 3

#define OPEN_FOUR 1
#define FOUR 2
#define OPEN_THREE 3
#define THREE 4
#define OPEN_TWO 5
#define TWO 6

#define FASTBOARD_CONTAINER 225

#define GOMOKU_TYPE_CONTAINER 100000

#define CLOCKS_PER_SEC ((clock_t)1000)

typedef unsigned long long U64;
typedef int(*MOVE)(int, int);
typedef std::unordered_set<int> ISET;
typedef std::vector<int> IVEC;

/////////////Fast Board/////////////////////
class FastBoard
{
public:
	int player = BLACK;
	int step = 0;
	bool is_over = false;
	int winner = -1;
	int history[STONES+1];
	U64 zobristKey;

	FastBoard();
	FastBoard(IVEC history, bool check = true);
	FastBoard(const FastBoard &copyFastBoard);

	void move(int action, bool check = true);
	IVEC get_history();
	IVEC get_board();

	IVEC get_actions(bool is_player, int gomoku_type);
	int count_actions(bool is_player, int gomoku_type);
	void get_fast_actions(bool is_player, int gomoku_type, int container[], int &count, int begin_index = 0);
	bool check_action(bool is_player, int gomoku_type, int action);

private:
	int _board[BOARD_SIZE*BOARD_SIZE];
	void reset();
	int gomoku_type_indice[11];
	int action_indice[11];
	unsigned char gomoku_types[FASTBOARD_CONTAINER];
	unsigned char gomoku_directions[FASTBOARD_CONTAINER];
	unsigned char actions[FASTBOARD_CONTAINER];
	void get_potential_actions();
};

int fastVct(FastBoard &fastBoard, int max_depth, double max_time);

#endif