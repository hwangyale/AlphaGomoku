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

#define GOMOKU_TYPE_CONTAINER 225
#define ACTION_CONTAINER 225
#define RANGE 9
#define GOMOKU_TABLE_CONTAINER 300000
//#define GOMOKU_TYPE_CONTAINER 100000

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
	GBIT Masks[4][BOARD_SIZE][BOARD_SIZE];
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
	bool get_actions(bool is_player, int gomoku_type, GBIT masked_line, UC container[], int begin, int &count);
	GomokuTypeTable();
private:
	static U64 gomoku_table[GOMOKU_TABLE_CONTAINER];
	bool load();
	bool save();

	bool get_four_actions();
	bool get_three_actions();
	bool get_two_actions();

};

class BitBoard
{
public:
	char player;
	UC step;
	bool is_over;
	char winner;
	U64 zobristKey;

	BitBoard();
	BitBoard(IVEC _history);
	BitBoard(std::vector<UC> history);
	BitBoard(const BitBoard &copyboard);
	BitBoard &operator = (const BitBoard &copyboard);
	~BitBoard();

	void move(UC action);

	IVEC get_board();
	IVEC get_actions(bool is_player, int gomoku_type);

	bool evaluate(UC actions[], int begin_index, int &count, int unknown = -1);

private:
	UC history[STONES + 1];
	int gomoku_indice[11];
	int action_indice[11];
	bool allocated;
	std::bitset<2 * BOARD_SIZE> bitboards[4][BOARD_SIZE];

	void allocate();
	void release();
	void reset();
	void copy(const BitBoard &copy_board);
	void check_gomoku_type();
	void get_fast_actions(bool is_player, int gomoku_type, UC container[], int begin, int &count);
	void count_actions(bool is_player, int gomoku_type);
	void check_action(bool is_player, int gomoku_type, UC action);
};

#endif