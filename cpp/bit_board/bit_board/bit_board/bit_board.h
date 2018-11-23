#ifndef _BIT_BOARD
#define _BIT_BOARD

#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <stdio.h>
#include <iostream>
#include "time.h"

#define BOARD_SIZE 15
#define HALF_SIZE 7
#define STONES 225
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define DRAW 3

#define BITBLACK 0
#define BITWHITE 1

#define OPEN_FOUR 1
#define FOUR 2
#define OPEN_THREE 3
#define THREE 4
#define OPEN_TWO 5

#define MAX_BOARD 1000000
#define GOMOKU_TYPE_CONTAINER 225
#define ACTION_CONTAINER 225
#define RANGE 9

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLOCKS_PER_SEC ((clock_t)1000)

typedef unsigned long long U64;
typedef unsigned long U32;
typedef int(*MOVE)(int, int);
typedef std::unordered_set<int> ISET;
typedef std::vector<int> IVEC;
typedef unsigned char UC;
typedef std::bitset<2 * BOARD_SIZE> GBIT;

inline char player_mapping(char player)
{
	return player == BLACK ? WHITE : BLACK;
}

class Table
{
public:
	GBIT BitBoard;
	GBIT Masks[2][BOARD_SIZE][BOARD_SIZE];
	U64 ZobristTable[2 * STONES + 4 + 10];

	Table();
	GBIT &get_mask(UC action, UC direction);

private:
	void init_BitBoard();
	void init_ZobristTable();
	void init_Masks();
};

class GomokuTypeTable
{
public:
	bool get_actions(bool is_player, int gomoku_type, GBIT &masked_line, int container[], int begin, int &count);
	GomokuTypeTable();
private:
	bool load();
	bool save();
	void generate_action_hash_table();
};

class BitBoard
{
public:
	bool allocated;
	char player;
	UC step;
	bool is_over;
	char winner;
	U64 zobristKey;

	BitBoard();
	BitBoard(bool not_allocated);
	BitBoard(IVEC _history);
	BitBoard(std::vector<UC> history);
	BitBoard(const BitBoard &copyboard);
	BitBoard &operator = (const BitBoard &copyboard);
	~BitBoard();

	GBIT &get_line(UC action, UC direction, int &center);
	void move(UC action);

	IVEC get_board();
	IVEC get_actions(bool is_player, int gomoku_type);

	int gomoku_indice[11];
	int action_indice[11];

	int evaluate(UC actions[], int begin_index, int &count, int current, 
			     std::unordered_map<U64, bool> &cache_table, int unknown = -1);

	void allocate();
	void release();
	void reset();

private:
	UC history[STONES + 1];
	GBIT bitboards[4][BOARD_SIZE];

	void copy(const BitBoard &copy_board);
	void check_gomoku_type();
	void get_fast_actions(bool is_player, int gomoku_type, UC container[], int begin, int &count);
	int count_actions(bool is_player, int gomoku_type);
	bool check_action(bool is_player, int gomoku_type, UC action);
};

int vct(BitBoard &board, int max_depth, double max_time);

#ifdef BOARD_TEST
class TestBoard
{
public:
	TestBoard();
	void test(BitBoard &board);
	void print();
private:
	int max_pointer = 0;
	int max_gomoku_type_number = 0;
	int max_action_number = 0;
};
#endif

#endif