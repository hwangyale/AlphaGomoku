#include "board.h"

void Board::set()
{
	is_over = bitboard.is_over;
	player = (int)bitboard.player;
	winner = (int)bitboard.winner;
	step = (int)bitboard.step;
	zobristKey = bitboard.zobristKey;
}

Board::Board()
{
	bitboard = BitBoard();
	set();
}

Board::Board(const Board &copyboard)
{
	bitboard = copyboard.bitboard;
	set();
}

void Board::move(int action)
{
	bitboard.move((UC)action);
	set();
}

std::vector<int> Board::get_history()
{
	std::vector<int> history(bitboard.history[0], 0);
	for (int i = 1; i <= bitboard.history[0]; i++)
	{
		history[i - 1] = (int)bitboard.history[i];
	}
	return history;
}

std::vector<int> Board::get_board()
{
	std::vector<int> board = bitboard.get_board();
	return board;
}

std::vector<int> Board::get_actions(bool is_player, int gomoku_type)
{
	std::vector<int> actions = bitboard.get_actions(is_player, gomoku_type);
	return actions;
}

int Board::Vct(int max_depth, double max_time)
{
	return vct(bitboard, max_depth, max_time);
}

