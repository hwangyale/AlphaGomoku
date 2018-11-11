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

/////////////Zobrist Table/////////////////
class ZobristTable
{
public:
	U64 keys[2 * STONES + 4 + 10];

	ZobristTable();
	inline U64 get_key(int color, int action);
};

ZobristTable::ZobristTable()
{
	U64 rand_number;
	srand(0);
	for (int i = 0; i < 2 * STONES + 4 + 10; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		keys[i] = rand_number;
	}
}

inline U64 ZobristTable::get_key(int color, int action)
{
	return keys[2 * action + color - 1];
}

ZobristTable zobristTable = ZobristTable();

/////////////FastBoardTable////////////////

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

void FastBoardTable::reset()
{
	count_table = std::unordered_map<U64, int>();
	board_table = std::unordered_map<U64, FastBoard>();
	keys.clear();
}

FastBoardTable::FastBoardTable()
{
	keys.reserve(max_size);
	reset();
}

bool FastBoardTable::check_table(U64 key, FastBoard &fastboard)
{
	if (board_table.find(key) != board_table.end())
	{
		fastboard = board_table[key];
		return true;
	}
	else if (count_table.find(key) != count_table.end())
	{
		count_table[key]++;
		return false;
	}
	else
	{
		count_table[key] = 1;
		keys.push_back(key);
		return false;
	}
}

void FastBoardTable::set_table(FastBoard &fastboard)
{
	U64 key = fastboard.zobristKey;
	if ((count_table.find(key) != count_table.end() && count_table[key] > threshold)
		&& board_table.find(key) == board_table.end())
	{
		if (keys.size() >= max_size)
		{
			int count = count_table[key];
			reset();
			count_table[key] = count;
			keys.push_back(key);
		}
		board_table[key] = fastboard;
	}
}

FastBoardTable FASTBOARDTABLE = FastBoardTable();


///////////////////FastBoard///////////////////////////

void FastBoard::reset()
{
	memset(_board, 0, sizeof(_board));
	zobristKey = 0;
	player = BLACK;
	history[0] = 0;
	is_over = false;
	winner = -1;
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
			//key ^= zobrist[2 * history[i]];
			key ^= zobristTable.get_key(BLACK, history[i]);
		}
		for (i = 1; i < history_length; i += 2)
		{
			//key ^= zobrist[2 * history[i] + 1];
			key ^= zobristTable.get_key(WHITE, history[i]);
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

	player = copyFastBoard.player;
	step = copyFastBoard.step;
	is_over = copyFastBoard.is_over;
	winner = copyFastBoard.winner;
	memcpy(history, copyFastBoard.history, (copyFastBoard.history[0] + 1) * sizeof(int));
	zobristKey = copyFastBoard.zobristKey;
	memcpy(_board, copyFastBoard._board, sizeof(_board));

	memcpy(gomoku_type_indice, copyFastBoard.gomoku_type_indice, sizeof(gomoku_type_indice));
	if (gomoku_type_indice[10] > 0)
	{
		memcpy(gomoku_types, copyFastBoard.gomoku_types, gomoku_type_indice[10] * sizeof(unsigned char));
		memcpy(gomoku_directions, copyFastBoard.gomoku_directions, gomoku_type_indice[10] * sizeof(unsigned char));
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
		//U64 key = zobristKey ^ zobrist[2 * action + (player == BLACK ? 0 : 1)];
		U64 key = zobristKey ^ zobristTable.get_key(player, action);
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
		//zobristKey ^= zobrist[2 * action + (player == BLACK ? 0 : 1)];
		zobristKey ^= zobristTable.get_key(player, action);
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

int FastBoard::count_actions(bool is_player, int gomoku_type)
{
	int index = ((is_player && player == BLACK) || (!is_player && player == WHITE) ? 0 : 5) + gomoku_type;
	return action_indice[index] - action_indice[index - 1];
}

void FastBoard::get_fast_actions(bool is_player, int gomoku_type, int container[], int &count, int begin_index)
{
	int index = ((is_player && player == BLACK) || (!is_player && player == WHITE) ? 0 : 5) + gomoku_type;
	int begin = action_indice[index - 1], end = action_indice[index];
	for (int i = 0; i < end - begin; i++)
	{
		count++;
		container[begin_index + i] = (int)actions[begin + i];
	}
}

bool FastBoard::check_action(bool is_player, int gomoku_type, int action)
{
	unsigned char act = (unsigned char)action;
	int index = ((is_player && player == BLACK) || (!is_player && player == WHITE) ? 0 : 5) + gomoku_type;
	int end = action_indice[index];
	for (int i = action_indice[index - 1]; i < end; i++)
	{
		if (actions[i] == act)
		{
			return true;
		}
	}
	return false;
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

//std::vector<U64> FastBoard::initZobrist()
//{
//	std::vector<U64> _zobrist;
//	U64 rand_number;
//	srand(0);
//	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE * 2; i++)
//	{
//		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
//		_zobrist.push_back(rand_number);
//	}
//	return _zobrist;
//}
//std::vector<U64> FastBoard::zobrist(FastBoard::initZobrist());

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
	int actions[100] = { 0 };
	for (int is_player = 1; is_player >= 0; is_player--)
	{
		std::cout << "is player: " << is_player << std::endl;
		for (int gt = OPEN_FOUR; gt <= OPEN_TWO; gt++)
		{
			/*actions[0] = 0;
			board.get_fast_actions((bool)is_player, gt, actions + 1, actions[0]);
			if (actions[0] == 0)
			{
				continue;
			}
			std::cout << "gomoku type: " << gt << std::endl << " | ";
			for (int idx = 1; idx <= actions[0]; idx++)
			{
				std::cout << actions[idx] / BOARD_SIZE << " " << actions[idx] % BOARD_SIZE << " | ";
			}
			std::cout << std::endl;*/

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

int defend(FastBoard &board)
{
	int actions[100] = { 0 };
	board.get_fast_actions(true, OPEN_FOUR, actions, actions[0], 1);
	board.get_fast_actions(true, FOUR, actions, actions[0], actions[0] + 1);
	if (actions[0] > 0)
	{
		return actions[1];
	}
	
	board.get_fast_actions(false, OPEN_FOUR, actions, actions[0], 1);
	board.get_fast_actions(false, FOUR, actions, actions[0], actions[0] + 1);
	//std::cout << actions[0] << " " << actions[actions[0]] << std::endl;
	if (actions[0] > 0)
	{
		return actions[1];
	}

	board.get_fast_actions(true, OPEN_THREE, actions, actions[0], 1);
	board.get_fast_actions(true, THREE, actions, actions[0], actions[0] + 1);
	if (actions[0] > 0)
	{
		return actions[1];
	}

	board.get_fast_actions(false, OPEN_THREE, actions, actions[0], 1);
	if (actions[0] > 0)
	{
		return actions[1];
	}

	return -1;

}
#endif // FAST_DEBUG


#ifdef FAST_DEBUG
void main()
{
	int _actions[16] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
		5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
		8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5 };
	/*int _actions[17] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
		5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
		8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5, 10 * 15 + 4 };*/
	/*int _actions[27] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
					     5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
						 8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5, 8 * 15 + 8, 9 * 15 + 9, 
						 8 * 15 + 9, 8 * 15 + 10, 5* 15 + 3, 5 * 15 + 4, 7 * 15 + 5, 6 * 15 + 4, 
						 7 * 15 + 4, 7 * 15 + 3, 4 * 15 + 8};*/
	IVEC actions(_actions, _actions + 16);

	FastBoard board;

	for (int i = 0; i < 16; i++)
	{
		board.move(actions[i], false);
		/*if (i < 16) continue;

		print_board(board);

		std::cout << "player: " << board.player << std::endl;

		print_potential_actions(board);

		std::cout << std::endl;
		getchar();*/
	}

	print_board(board);

	int step = 0, action;

	while (!board.is_over)
	{
		if ((++step) % 2 == 1)
		{
			action = fastVct(board, 12, 3600);
			if (action < 0)
			{
				std::cout << "vct failed" << std::endl;
				break;
			}
			std::cout << "vct action: " << action / BOARD_SIZE << " " << action % BOARD_SIZE << std::endl;
		}
		else
		{
			action = defend(board);
			if (action < 0)
			{
				std::cout << "vct failed" << std::endl;
				break;
			}
			std::cout << "defend action: " << action / BOARD_SIZE << " " << action % BOARD_SIZE << std::endl;
		}
		board.move(action, false);

		print_board(board);

		std::cout << "player: " << board.player << std::endl;

		print_potential_actions(board);

		std::cout << std::endl;

		getchar();
	}

	/*print_board(board);

	std::cout << "player: " << board.player << std::endl;

	clock_t start = clock();

	move_time = copy_time = 0.0;
	extern double check_time, five_time, four_time, three_time, two_time, get_actions_time;
	extern double set_proof_and_disproof_time, evaluate_time;
	check_time = five_time = four_time = three_time = two_time = get_actions_time = 0.0;
	set_proof_and_disproof_time = evaluate_time = 0.0;

	std::cout << fastVct(board, 12, 3600.0) << std::endl;

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