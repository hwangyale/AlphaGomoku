#ifndef _BOARD
#define _BOARD

#include "bit_board.h"

class Board
{
public:
	BitBoard bitboard;
	bool is_over;
	int player;
	int winner;
	int step;
	U64 zobristKey;

	Board();
	Board(const Board &copyboard);
	void move(int action);
	std::vector<int> get_history();
	std::vector<int> get_board();
	std::vector<int> get_actions(bool is_player, int gomoku_type);
	int Vct(int max_depth, double max_time);
private:
	void set();
};


#endif
