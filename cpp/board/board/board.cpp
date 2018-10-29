#include "board.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "time.h"

std::vector<int> get_elements(ISET &set)
{
	std::vector<int> list;
	for (ISET::iterator idx = set.begin(); idx != set.end(); idx++)
	{
		list.push_back(*idx);
	}
	return list;
}

void Board::reset()
{
	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; i++)
	{
		_board[i] = EMPTY;
	}
	zobristKey = 0;
	player = BLACK;
	history = std::vector<int>();
	is_over = false;
	winner = -1;

	for (int color = BLACK; color <= WHITE; color++)
	{
		for (int gt = OPEN_FOUR; gt <= TWO; gt++)
		{
			gomoku_types[color - 1][gt - 1] = ISET();
			poses[color - 1][gt - 1] = ISET();
		}
	}
}

Board::Board()
{
	reset();
}

Board::Board(std::vector<int> poses)
{
	reset();
	for (int i = 0; i < (int)poses.size(); i++)
	{
		move(poses[i]);
	}
}

#ifdef DEBUG
double copy_time = 0.0;
#endif // DEBUG


Board::Board(const Board &copy_board)
{
	clock_t start = clock();

	player = copy_board.player;
	step = copy_board.step;
	is_over = copy_board.is_over;
	winner = copy_board.winner;
	history = std::vector<int>();
	history.insert(history.begin(), copy_board.history.begin(), copy_board.history.end());
	zobristKey = copy_board.zobristKey;

	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; i++)
	{
		_board[i] = copy_board._board[i];
	}
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			gomoku_types[i][j] = copy_board.gomoku_types[i][j];
			poses[i][j] = copy_board.poses[i][j];
		}
	}

	#ifdef DEBUG
		copy_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

}

extern void get_promising_positions(int _board[], ISET gomoku_types[2][6], ISET positions[12], int position, int player, bool check_two);

#ifdef DEBUG

double move_time = 0.0;

#endif // DEBUG

extern bool check_five(int _board[], int position, int color, MOVE move_func);
extern MOVE move_list[4];

void Board::move(int pos, bool check_two)
{
	clock_t start = clock();

	bool flag = false;
	for (int func_idx = 0; func_idx < 4; func_idx++)
	{
		if (check_five(_board, pos, player, move_list[func_idx]))
		{
			flag = true;
			break;
		}
	}

	if (flag)
	{
		winner = player;
		is_over = true;
	}
	
	_board[pos] = player;
	step++;
	if (!is_over && step == STONES)
	{
		is_over = true;
		winner = EMPTY;
	}
	if (player == BLACK)
	{
		player = WHITE;
	}
	else
	{
		player = BLACK;
	}
	history.push_back(pos);
	
	int idx = 2 * pos;
	if (player == WHITE)
	{
		idx++;
	}
	zobristKey ^= zobrist[idx];

	//get_promising_positions(_board, gomoku_types, poses, pos, player, check_two);
	ISET positions[12];
	get_promising_positions(_board, gomoku_types, positions, pos, player, check_two);
	for (int i = 0; i <= 1; i++)
	{
		for (int j = 0; j <= 5; j++)
		{
			poses[i][j] = positions[i * 6 + j];
		}
	}

	#ifdef DEBUG
	move_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG
	
}

ISET Board::_get_positions(bool is_player, int gomoku_type)
{
	int i = is_player ? 0 : 1;
	return poses[i][gomoku_type - 1];
}

std::vector<int> Board::get_positions(bool is_player, int gomoku_type)
{
	return get_elements(_get_positions(is_player, gomoku_type));
}

std::vector<int> Board::get_board()
{
	std::vector<int> board_list(_board, _board + BOARD_SIZE*BOARD_SIZE);
	return board_list;
}

std::vector<U64> Board::initZobrist()
{
	std::vector<U64> _zobrist;
	U64 rand_number;
	srand(0);
	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE*2; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		_zobrist.push_back(rand_number);
	}
	return _zobrist;
}
std::vector<U64> Board::zobrist(Board::initZobrist());

#ifdef DEBUG
void main()
{
	int _poses[16] = { 7*15+7, 6*15+7, 6*15+8, 7*15+8, 6*15+6, 6*15+5, 
					   5*15+6, 4*15+6, 5*15+5, 4*15+4, 7*15+6, 5*15+9, 
					   8*15+6, 9*15+6, 8*15+7, 8*15+5};
	std::vector<int> poses(_poses, _poses + 16);
	Board board(poses);
	std::vector<int> _board = board.get_board();
	
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			std::cout << _board[i*BOARD_SIZE + j] << ' ';
		}
		std::cout << std::endl;
	}

	clock_t start = clock();

	move_time = copy_time = 0.0;
	extern double check_time, five_time, four_time, three_time, two_time, get_positions_time;
	extern double set_proof_and_disproof_time, evaluate_time;
	check_time = five_time = four_time = three_time = two_time = get_positions_time = 0.0;
	set_proof_and_disproof_time = evaluate_time = 0.0;

	std::cout << vct(board, 100, 100.0) << std::endl;
	//std::cout << pvs(board, 20, 100) << std::endl;
	
	/*for (int i = 0; i <= 1; i++)
	{
		bool is_player = i == 0 ? true : false;
		std::cout << "is_player: " << is_player << std::endl;
		for (int gt = OPEN_FOUR; gt <= TWO; gt++)
		{
			std::cout << "\n" << "gomoku type: " << gt << std::endl;
			ISET positions = board._get_positions(is_player, gt);
			for (ISET::iterator idx = positions.begin(); idx != positions.end(); idx++)
			{
				std::cout << "  " << *idx / BOARD_SIZE << " " << *idx % BOARD_SIZE << std::endl;
			}
		}
	}*/

	std::cout << "total time: " << (double)(clock() - start) / CLOCKS_PER_SEC << std::endl;
	std::cout << "check time: " << check_time << std::endl;
	std::cout << "move time: " << move_time << std::endl;
	std::cout << "five time: " << five_time << std::endl;
	std::cout << "four time: " << four_time << std::endl;
	std::cout << "three time: " << three_time << std::endl;
	std::cout << "two time: " << two_time << std::endl;
	std::cout << "get_positions time: " << get_positions_time << std::endl;
	std::cout << "copy time: " << copy_time << std::endl;
	std::cout << "set proof and disproof time: " << set_proof_and_disproof_time << std::endl;
	std::cout << "evaluate time: " << evaluate_time << std::endl;
	getchar();
}

#endif