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

#define FASTBOARD_CONTAINER 100

#define CLOCKS_PER_SEC ((clock_t)1000)

typedef unsigned long long U64;
typedef int(*MOVE)(int, int);
typedef std::unordered_set<int> ISET;
typedef std::vector<int> IVEC;

class Board
{
public:
	int player = BLACK;
	int step = 0;
	bool is_over = false;
	int winner = -1;
	std::vector<int> history;
	U64 zobristKey;
	static std::vector<U64> zobrist;
	static std::vector<U64> initZobrist();

	Board();
	Board(std::vector<int> poses);
	Board(const Board &_board);

	void move(int pos, bool check_two=true);
	std::vector<int> get_board();

	std::vector<int> get_positions(bool is_player, int gomoku_type);
	ISET _get_positions(bool is_player, int gomoku_type);

private:
	int _board[BOARD_SIZE*BOARD_SIZE];
	void reset();
	ISET gomoku_types[2][6];
	ISET poses[2][6];
};

std::vector<int> get_elements(ISET &set);

int vct(Board &board, int max_depth, double max_time);


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
	static std::vector<U64> zobrist;
	static std::vector<U64> initZobrist();

	FastBoard();
	FastBoard(IVEC history, bool check = true);
	FastBoard(const FastBoard &copyFastBoard);

	void move(int action, bool check = true);
	IVEC get_history();
	IVEC get_board();

	IVEC get_actions(bool is_player, int gomoku_type);

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

class FastBoardTable
{
public:
	bool check_table(U64 key, FastBoard &fastboard);
	void set_table(FastBoard &fastboard);
	FastBoardTable();
private:
	static const int threshold = 10;
	static const int max_size = 10000;
	std::unordered_map<U64, int> count_table;
	std::unordered_map<U64, FastBoard> board_table;
	std::vector<U64> keys;
	void reset();
};

int fastVct(FastBoard &fastBoard, int max_depth, double max_time);

#endif