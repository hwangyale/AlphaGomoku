#include "board.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "time.h"
#define SINGLE_BOARD_CONTAINER 10

FastBoardTable FASTBOARDTABLE = FastBoardTable();

void FastBoard::reset()
{
	memset(_board, 0, sizeof(_board));
	zobristKey = 0;
	player = BLACK;
	history[0] = 0;
	is_over = false;
	winner = -1;
	gomoku_type_container_count = 10;
	action_container_count = 10;
	memset(gomoku_types, 0, gomoku_type_container_count * sizeof(char));
	memset(gomoku_directions, 0, gomoku_type_container_count * sizeof(char));
	memset(actions, 0, action_container_count * sizeof(char));

	/*int i, j;
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 5; j++)
		{
			gomoku_types[i][j].reserve(SINGLE_BOARD_CONTAINER);
			gomoku_types[i][j].clear();
			actions[i][j].reserve(SINGLE_BOARD_CONTAINER);
			actions[i][j].clear();
		}
	}*/
}

FastBoard::FastBoard()
{
	reset();
}

FastBoard::FastBoard(IVEC history, bool check)
{
	int i, history_length = (int)history.size();
	if (check)
	{
		U64 key = 0;
		for (i = 1; i <= history_length; i += 2)
		{
			key ^= zobrist[2 * history[i]];
		}
		for (i = 2; i <= history_length; i += 2)
		{
			key ^= zobrist[2 * history[i] + 1];
		}
		FASTBOARDTABLE.check_table(key, *this);
	}
	else
	{
		reset();
		for (i = 1; i <= history_length; i++)
		{
			move(history[i], false);
		}
	}
}

#ifdef FAST_DEBUG
double copy_time = 0.0;
#endif // FAST_DEBUG

FastBoard::FastBoard(const FastBoard &copyFastBoard)
{
	#if FAST_DEBUG
	clock_t start = clock();
	#endif // FAST_DEBUG

	int i, j;

	player = copyFastBoard.player;
	step = copyFastBoard.step;
	is_over = copyFastBoard.is_over;
	winner = copyFastBoard.winner;
	history[0] = copyFastBoard.history[0];
	for (i = 1; i <= history[0]; i++)
	{
		history[i] = copyFastBoard.history[i];
	}
	zobristKey = copyFastBoard.zobristKey;

	for (i = 0; i < BOARD_SIZE*BOARD_SIZE; i++)
	{
		_board[i] = copyFastBoard._board[i];
	}
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 5; j++)
		{
			gomoku_types[i][j] = copyFastBoard.gomoku_types[i][j];
			actions[i][j] = copyFastBoard.actions[i][j];
		}
	}

	#ifdef FAST_DEBUG
	copy_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

}

extern void get_potential_actions(int _board[], IVEC gomoku_types[2][5], IVEC actions[2][5], 
							      int action, int player);

#ifdef FAST_DEBUG

double move_time = 0.0;

#endif // FAST_DEBUG

extern bool check_fast_five(int _board[], int action, int color, MOVE move_func);
extern MOVE move_list[4];

void FastBoard::move(int action, bool check)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif // FAST_DEBUG

	bool move_flag = true;
	if (check)
	{
		U64 key = zobristKey ^ zobrist[2 * action + (player == BLACK ? 0 : 1)];
		move_flag = !(FASTBOARDTABLE.check_table(key, *this));
	}

	if (move_flag)
	{
		bool flag = false;
		for (int func_idx = 0; func_idx < 4; func_idx++)
		{
			if (check_fast_five(_board, action, player, move_list[func_idx]))
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

		_board[action] = player;
		step++;
		if (!is_over && step == STONES)
		{
			is_over = true;
			winner = EMPTY;
		}
		history[++history[0]] = action;
		zobristKey ^= zobrist[2 * action + (player == BLACK ? 0 : 1)];
		player = player == BLACK ? WHITE : BLACK;
		get_potential_actions(_board, gomoku_types, actions, action, player);

		if (check)
		{
			FASTBOARDTABLE.set_table(*this);
		}
	}

	#ifdef FAST_DEBUG
	move_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG
}

IVEC FastBoard::get_actions(bool is_player, int gomoku_type)
{
	return actions[(is_player ? 0 : 1)][gomoku_type-1];
}

IVEC FastBoard::get_history()
{
	return IVEC(history + 1, history + 1 + history[0]);
}

IVEC FastBoard::get_board()
{
	IVEC board_list(_board, _board + BOARD_SIZE*BOARD_SIZE);
	return board_list;
}

std::vector<U64> FastBoard::initZobrist()
{
	std::vector<U64> _zobrist;
	U64 rand_number;
	srand(0);
	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE * 2; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		_zobrist.push_back(rand_number);
	}
	return _zobrist;
}
std::vector<U64> FastBoard::zobrist(FastBoard::initZobrist());

#ifdef FAST_DEBUG
void main()
{
	int _actions[16] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
		5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
		8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5 };
	IVEC actions(_actions, _actions + 16);
	FastBoard board(actions);
	IVEC _board = board.get_board();

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			std::cout << _board[i*BOARD_SIZE + j] << ' ';
		}
		std::cout << std::endl;
	}

	/*clock_t start = clock();

	move_time = copy_time = 0.0;
	extern double check_time, five_time, four_time, three_time, two_time, get_actions_time;
	extern double set_proof_and_disproof_time, evaluate_time;
	check_time = five_time = four_time = three_time = two_time = get_actions_time = 0.0;
	set_proof_and_disproof_time = evaluate_time = 0.0;

	std::cout << vct(board, 100, 100.0) << std::endl;

	std::cout << "total time: " << (double)(clock() - start) / CLOCKS_PER_SEC << std::endl;
	std::cout << "check time: " << check_time << std::endl;
	std::cout << "move time: " << move_time << std::endl;
	std::cout << "five time: " << five_time << std::endl;
	std::cout << "four time: " << four_time << std::endl;
	std::cout << "three time: " << three_time << std::endl;
	std::cout << "two time: " << two_time << std::endl;
	std::cout << "get_actions time: " << get_actions_time << std::endl;
	std::cout << "copy time: " << copy_time << std::endl;
	std::cout << "set proof and disproof time: " << set_proof_and_disproof_time << std::endl;
	std::cout << "evaluate time: " << evaluate_time << std::endl;*/
	getchar();
}

#endif