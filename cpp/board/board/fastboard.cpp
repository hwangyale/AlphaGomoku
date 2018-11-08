#include "board.h"
#include <stdio.h>
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
	int i;
	/*for (i = 0; i < STONES; i++)
	{
		_board[i] = EMPTY;
	}*/
	memset(_board, 0, sizeof(_board));
	zobristKey = 0;
	player = BLACK;
	history[0] = 0;
	is_over = false;
	winner = -1;
	/*for (i = 0; i < 11; i++)
	{
		gomoku_type_indice[i] = 0;
		action_indice[i] = 0;
	}*/
	memset(gomoku_type_indice, 0, sizeof(gomoku_type_indice));
	memset(action_indice, 0, sizeof(action_indice));
}

FastBoard::FastBoard()
{
	reset();
}

FastBoard::FastBoard(IVEC history, bool check)
{
	int i, history_length = (int)history.size();
	bool in_table = false;
	if (check)
	{
		U64 key = 0;
		for (i = 0; i < history_length; i += 2)
		{
			key ^= zobrist[2 * history[i]];
		}
		for (i = 1; i < history_length; i += 2)
		{
			key ^= zobrist[2 * history[i] + 1];
		}
		in_table = FASTBOARDTABLE.check_table(key, *this);
	}

	if (!in_table)
	{
		reset();
		for (i = 0; i < history_length; i++)
		{
			move(history[i], check);
		}
	}

	if (check)
	{
		FASTBOARDTABLE.set_table(*this);
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

	int i;

	player = copyFastBoard.player;
	step = copyFastBoard.step;
	is_over = copyFastBoard.is_over;
	winner = copyFastBoard.winner;
	/*for (i = 0; i <= copyFastBoard.history[0]; i++)
	{
		history[i] = copyFastBoard.history[i];
	}*/
	memcpy(history, copyFastBoard.history, (copyFastBoard.history[0] + 1) * sizeof(int));
	zobristKey = copyFastBoard.zobristKey;
	/*for (i = 0; i < STONES; i++)
	{
		_board[i] = copyFastBoard._board[i];
	}*/
	memcpy(_board, copyFastBoard._board, sizeof(_board));

	/*for (i = 0; i < 11; i++)
	{
		gomoku_type_indice[i] = copyFastBoard.gomoku_type_indice[i];
		action_indice[i] = copyFastBoard.action_indice[i];
	}

	for (i = 0; i < copyFastBoard.gomoku_type_indice[10]; i++)
	{
		gomoku_types[i] = copyFastBoard.gomoku_types[i];
		gomoku_directions[i] = copyFastBoard.gomoku_directions[i];
	}

	for (i = 0; i < copyFastBoard.action_indice[10]; i++)
	{
		actions[i] = copyFastBoard.actions[i];
	}*/

	memcpy(gomoku_type_indice, copyFastBoard.gomoku_type_indice, sizeof(gomoku_type_indice));
	if (gomoku_type_indice[10] > 0)
	{
		memcpy(gomoku_types, copyFastBoard.gomoku_types, gomoku_type_indice[10] * sizeof(unsigned char));
	}

	memcpy(action_indice, copyFastBoard.action_indice, sizeof(action_indice));
	if (action_indice[10] > 0)
	{
		memcpy(actions, copyFastBoard.actions, action_indice[10] * sizeof(unsigned char));
	}

	#ifdef FAST_DEBUG
	copy_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

}

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
		get_potential_actions();

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
	int index = ((is_player && player == BLACK) || (!is_player && player == WHITE) ? 0 : 5) + gomoku_type;
	IVEC _actions(actions + action_indice[index - 1], actions + action_indice[index]);
	return _actions;
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
void print_board(FastBoard &board)
{
	IVEC _board = board.get_board();
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		printf("%2d ", i);
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			if (_board[i*BOARD_SIZE + j] == EMPTY)
			{
				std::cout << "_" << ' ';
			}
			else
			{
				std::cout << _board[i*BOARD_SIZE + j] << ' ';
			}

		}
		std::cout << std::endl;
	}
	printf("   ");
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (i < 10)
		{
			printf("%d ", i);
		}
		else
		{
			printf("1 ");
		}
	}
	printf("\n   ");
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (i < 10)
		{
			printf("  ");
		}
		else
		{
			printf("%d ", i - 10);
		}
	}
	printf("\n");
}

void print_potential_actions(FastBoard &board)
{
	for (int is_player = 1; is_player >= 0; is_player--)
	{
		std::cout << "is player: " << is_player << std::endl;
		for (int gt = OPEN_FOUR; gt <= OPEN_TWO; gt++)
		{

			IVEC potential_actions = board.get_actions((bool)is_player, gt);

			if (potential_actions.size() == 0)
			{
				continue;
			}
			std::cout << "gomoku type: " << gt << std::endl << " | ";
			for (IVEC::iterator act = potential_actions.begin(); act != potential_actions.end(); act++)
			{
				std::cout << (*act) / BOARD_SIZE << " " << (*act) % BOARD_SIZE << " | ";
			}
			std::cout << std::endl;
		}
	}
}
#endif // FAST_DEBUG


#ifdef FAST_DEBUG
void main()
{
	int _actions[16] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
		5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
		8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5 };
	IVEC actions(_actions, _actions + 16);

	FastBoard board;

	for (int i = 0; i < 16; i++)
	{
		board.move(actions[i]);
		print_board(board);

		std::cout << "player: " << board.player << std::endl;

		print_potential_actions(board);

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