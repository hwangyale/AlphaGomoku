#include "board.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

#define CHECK_SIZE 8

extern MOVE move_list[4];

extern int player_mapping(int player);

#ifdef FAST_DEBUG

double check_time = 0.0;
double five_time = 0.0;
double four_time = 0.0;
double three_time = 0.0;
double two_time = 0.0;
double get_actions_time = 0.0;

#endif // FAST_DEBUG

void fast_check(int cache_counts[], int cache_actions[],
			    int _board[], int action, int color,
				MOVE move_func, int max_empty_count, int max_delta = 6)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif

	int empty_count, tmp_action, delta, opponent = player_mapping(color);
	memset(cache_counts, 0, sizeof(cache_counts));
	std::fill_n(cache_actions, 2 * max_empty_count - 1, -1);
	cache_counts[max_empty_count - 1] = 1;
	cache_actions[max_empty_count - 1] = action;
	for (int sign = -1; sign <= 1; sign += 2)
	{
		empty_count = 0;
		for (delta = 1; delta < max_delta; delta++)
		{
			tmp_action = move_func(action, sign*delta);
			if (tmp_action < 0
				|| (_board[tmp_action] == opponent)
				|| empty_count == max_empty_count)
			{
				break;
			}
			if (_board[tmp_action] == color)
			{
				cache_counts[sign*empty_count + max_empty_count - 1] += 1;
			}
			else
			{
				empty_count += 1;
				if (empty_count == max_empty_count)
				{
					break;
				}
				cache_actions[sign*empty_count + max_empty_count - 1] = tmp_action;
			}
		}
	}


	#ifdef FAST_DEBUG
	check_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

}

std::unordered_map<U64, IVEC> GOMOKU_TYPE_TABLE;
U64 get_action_key(int gomoku_type, bool is_player, int cache_counts[], int cache_actions[], int max_empty_count)
{
	static U64 base = 1;
	static U64 Radix = base << 3;
	static U64 Prefix = base << 60;
	U64 key = 0;

	key += (is_player ? 1 : 0) + 1;
	for (int idx = 0; idx < 2 * max_empty_count - 1; idx++)
	{
		key = key * Radix + (cache_counts[idx] >= 6 ? 6 : cache_counts[idx]) + 1;
		key = key * Radix + (cache_actions[idx] < 0 ? 0 : 1) + 1;
	}
	return gomoku_type * Prefix + key;
}


bool check_fast_five(int _board[], int action, int color, MOVE move_func)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif // FAST_DEBUG

	int cache_counts[CHECK_SIZE], cache_actions[CHECK_SIZE];
	fast_check(cache_counts, cache_actions, _board, action, color, move_func, 1);

	#ifdef FAST_DEBUG
	five_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (cache_counts[0] >= 5)
	{
		return true;
	}
	else
	{
		return false;
	}
}

ActionHash ActionTable = ActionHash();
int BEGIN;

bool check_fast_open_four_and_four(int _board[], int action, int color, MOVE move_func,
	bool is_open, bool is_player, int &begin = BEGIN, unsigned char *actions = NULL)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif

	int index, max_empty_count = 3, cache_action, cache_counts[CHECK_SIZE], cache_actions[CHECK_SIZE];
	fast_check(cache_counts, cache_actions, _board, action, color, move_func, max_empty_count, 7);

	U64 key = get_action_key((is_open ? OPEN_FOUR : FOUR), is_player, cache_counts, cache_actions, max_empty_count);

	if (GOMOKU_TYPE_TABLE.find(key) == GOMOKU_TYPE_TABLE.end())
	{
		int _indice[CHECK_SIZE] = { 0 };
		for (int i = 1; i < max_empty_count; i++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				index = sign * i + max_empty_count - 1;
				cache_action = cache_actions[index];
				if (cache_action < 0)
				{
					continue;
				}
				_board[cache_action] = color;
				if (check_fast_five(_board, action, color, move_func))
				{
					_indice[++_indice[0]] = index;
				}
				_board[cache_action] = EMPTY;
			}
		}

		IVEC indice;
		if ((is_open && _indice[0] >= 2) || (!is_open && _indice[0] == 1))
		{
			for (int idx = 1; idx <= _indice[0]; idx++)
			{
				indice.push_back(_indice[idx]);
			}
		}

		GOMOKU_TYPE_TABLE.insert(std::pair<U64, IVEC>(key, indice));
	}

	IVEC &indice = GOMOKU_TYPE_TABLE[key];
	if (actions != NULL && !indice.empty())
	{
		int check_gt = is_open ? OPEN_FOUR : FOUR;
		for (IVEC::iterator idx = indice.begin(); idx != indice.end(); idx++)
		{
			if (!ActionTable.check(check_gt, cache_actions[*idx], color))
			{
				actions[begin++] = (unsigned char)cache_actions[*idx];
			}
		}
	}

	#ifdef FAST_DEBUG
	four_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (!indice.empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool check_fast_open_three_and_three(int _board[], int action, int color, MOVE move_func,
	bool is_open, bool is_player, int &begin = BEGIN, unsigned char *actions = NULL)
{
	clock_t start = clock();

	int index, max_empty_count = 3, cache_action, cache_counts[CHECK_SIZE], cache_actions[CHECK_SIZE];
	fast_check(cache_counts, cache_actions, _board, action, color, move_func, max_empty_count);

	U64 key = get_action_key((is_open ? OPEN_THREE : THREE), is_player, cache_counts, cache_actions, max_empty_count);

	if (GOMOKU_TYPE_TABLE.find(key) == GOMOKU_TYPE_TABLE.end())
	{
		IVEC indice;
		int attack_action_indice[CHECK_SIZE] = { 0 };
		for (int i = 1; i < max_empty_count; i++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				index = sign * i + max_empty_count - 1;
				cache_action = cache_actions[index];
				if (cache_action < 0)
				{
					continue;
				}
				_board[cache_action] = color;
				if (check_fast_open_four_and_four(_board, action, color, move_func, is_open, true))
				{
					if (is_player)
					{
						indice.push_back(index);
					}
					else
					{
						attack_action_indice[++attack_action_indice[0]] = index;
					}
				}
				_board[cache_action] = EMPTY;
			}
		}
		if (!is_player && attack_action_indice[0] > 0)
		{
			int player = player_mapping(color), _cache_action;
			bool flag;
			for (int i = 1; i < max_empty_count; i++)
			{
				for (int sign = -1; sign <= 1; sign += 2)
				{
					index = sign * i + max_empty_count - 1;
					cache_action = cache_actions[index];
					if (cache_action < 0)
					{
						continue;
					}
					_board[cache_action] = player;

					flag = true;
					for (int idx = 1; idx <= attack_action_indice[0]; idx++)
					{
						if (attack_action_indice[idx] == index)
						{
							continue;
						}
						_cache_action = cache_actions[attack_action_indice[idx]];
						_board[_cache_action] = color;
						if (check_fast_open_four_and_four(_board, action, color, move_func, is_open, true))
						{
							_board[_cache_action] = EMPTY;
							_board[cache_action] = EMPTY;
							flag = false;
							break;
						}
						_board[_cache_action] = EMPTY;
					}
					if (flag)
					{
						_board[cache_action] = EMPTY;
						indice.push_back(index);
					}
				}
			}
		}

		GOMOKU_TYPE_TABLE.insert(std::pair<U64, IVEC>(key, indice));
	}

	IVEC &indice = GOMOKU_TYPE_TABLE[key];
	if (actions != NULL && !indice.empty())
	{
		int check_gt = is_open ? OPEN_THREE : THREE;
		for (IVEC::iterator idx = indice.begin(); idx != indice.end(); idx++)
		{
			if (!ActionTable.check(check_gt, cache_actions[*idx], color))
			{
				actions[begin++] = (unsigned char)cache_actions[*idx];
			}
		}
	}

	#ifdef FAST_DEBUG
	three_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (!indice.empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool check_fast_open_two(int _board[], int action, int color, MOVE move_func,
	bool is_open, bool is_player, int &begin =BEGIN, unsigned char *actions = NULL)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();

	#endif // FAST_DEBUG

	int index, max_empty_count = 4, cache_action, cache_counts[CHECK_SIZE], cache_actions[CHECK_SIZE];
	fast_check(cache_counts, cache_actions, _board, action, color, move_func, max_empty_count);

	U64 key = get_action_key(OPEN_TWO, is_player, cache_counts, cache_actions, max_empty_count);

	if (GOMOKU_TYPE_TABLE.find(key) == GOMOKU_TYPE_TABLE.end())
	{
		IVEC indice;
		int attack_action_indice[CHECK_SIZE] = { 0 };
		for (int i = 1; i < max_empty_count; i++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				index = sign * i + max_empty_count - 1;
				cache_action = cache_actions[index];
				if (cache_action < 0)
				{
					continue;
				}
				_board[cache_action] = color;
				if (check_fast_open_three_and_three(_board, action, color, move_func, true, true))
				{
					if (is_player)
					{
						indice.push_back(index);
					}
					else
					{
						attack_action_indice[++attack_action_indice[0]] = index;
					}
				}
				_board[cache_action] = EMPTY;
			}
		}

		if (!is_player && attack_action_indice[0] > 0)
		{
			int player = player_mapping(color), _cache_action;
			bool flag;
			for (int i = 1; i < max_empty_count; i++)
			{
				for (int sign = -1; sign <= 1; sign += 2)
				{
					index = sign * i + max_empty_count - 1;
					cache_action = cache_actions[index];
					if (cache_action < 0)
					{
						continue;
					}
					_board[cache_action] = player;

					flag = true;
					for (int idx = 1; idx <= attack_action_indice[0]; idx++)
					{
						if (attack_action_indice[idx] == index)
						{
							continue;
						}
						_cache_action = cache_actions[attack_action_indice[idx]];
						_board[_cache_action] = color;
						if (check_fast_open_three_and_three(_board, action, color, move_func, true, true))
						{
							_board[_cache_action] = EMPTY;
							_board[cache_action] = EMPTY;
							flag = false;
							break;
						}
						_board[_cache_action] = EMPTY;
					}
					if (flag)
					{
						_board[cache_action] = EMPTY;
						indice.push_back(index);
					}
				}
			}
		}

		GOMOKU_TYPE_TABLE.insert(std::pair<U64, IVEC>(key, indice));
	}

	IVEC &indice = GOMOKU_TYPE_TABLE[key];
	if (actions != NULL && !indice.empty())
	{
		int check_gt = OPEN_TWO;
		for (IVEC::iterator idx = indice.begin(); idx != indice.end(); idx++)
		{
			if (!ActionTable.check(check_gt, cache_actions[*idx], color))
			{
				actions[begin++] = (unsigned char)cache_actions[*idx];
			}
			//actions[begin++] = (unsigned char)cache_actions[*idx];
		}
	}

	#ifdef FAST_DEBUG
	two_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (!indice.empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}


typedef bool(*CHECK)(int[], int, int, MOVE, bool, bool, int &, unsigned char *);
CHECK check_funcs[3] = { check_fast_open_four_and_four, check_fast_open_three_and_three, check_fast_open_two };


U64 *get_fast_zobrist()
{
	U64 *_zobrist = new U64[STONES * 2 + 4 + 10];
	U64 rand_number;
	srand(0);
	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE * 2 + 4 + 10; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		_zobrist[i] = rand_number;
	}
	return _zobrist;
}

U64 *FAST_ZOBRIST_TABLE = get_fast_zobrist();

U64 get_fast_block_zobrisKey(int _board[], int action, int color, int func_idx)
{
	U64 key = FAST_ZOBRIST_TABLE[action + (color == BLACK ? 0 : STONES)] ^ FAST_ZOBRIST_TABLE[2 * STONES + func_idx];
	MOVE move_func = move_list[func_idx];
	int tmp_action;
	for (int sign = -1; sign <= 1; sign += 2)
	{
		for (int delta = 1; delta < 6; delta++)
		{
			//std::cout << "func_idx: " << func_idx << " action: " << action << std::endl;
			tmp_action = move_func(action, sign*delta);
			//std::cout << "tmp_action: " << tmp_action << std::endl;
			if (tmp_action >= 0 && _board[tmp_action] == color)
			{
				key ^= FAST_ZOBRIST_TABLE[tmp_action + (color == BLACK ? 0 : STONES)];
			}
			else
			{
				break;
			}
		}
	}
	return key;
}

GomokuTypeHash GomokuTypeTable = GomokuTypeHash();

void FastBoard::get_potential_actions()
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif

	int opponent = player_mapping(player), action = history[history[0]], act, func_idx, base, delta;
	int begin, end, index_begin, index_end, tmp_index, tmp_gomoku_type_indice[11], rest_nb;
	static unsigned char tmp_gomoku_types[FASTBOARD_CONTAINER], tmp_gomoku_directions[FASTBOARD_CONTAINER];
	static unsigned char tmp_types[FASTBOARD_CONTAINER], tmp_directions[FASTBOARD_CONTAINER];
	static unsigned char rest_actions_for_searching[FASTBOARD_CONTAINER], rest_directions_for_searching[FASTBOARD_CONTAINER];
	std::unordered_set<U64> block_coding_table;
	GomokuTypeHash GomokuTypeTable;
	U64 key, op_gt_key, gt_key;

	memcpy(tmp_gomoku_type_indice, gomoku_type_indice, sizeof(tmp_gomoku_type_indice));
	memcpy(tmp_gomoku_types, gomoku_types, gomoku_type_indice[10] * sizeof(unsigned char));
	memcpy(tmp_gomoku_directions, gomoku_directions, gomoku_type_indice[10] * sizeof(unsigned char));

	GomokuTypeTable.reset();
	ActionTable.reset();

	for (int color = BLACK; color <= WHITE; color++)
	{
		base = (color - 1) * 5;

		for (int op_gt = OPEN_FOUR; op_gt <= OPEN_TWO; op_gt += 2)
		{
			index_begin = tmp_gomoku_type_indice[base + op_gt - 1];
			index_end = tmp_gomoku_type_indice[base + op_gt];

			if (color == opponent)
			{
				for (int j = 0; j < 4; j++)
				{
					tmp_types[j] = (unsigned char)action;
					tmp_directions[j] = (unsigned char)j;
				}
				delta = 4;
			}
			else
			{
				delta = 0;
			}
			
			memcpy(tmp_types + delta, tmp_gomoku_types + index_begin, (index_end - index_begin) * sizeof(unsigned char));
			memcpy(tmp_directions + delta, tmp_gomoku_directions + index_begin, (index_end - index_begin) * sizeof(unsigned char));

			op_gt_key = FAST_ZOBRIST_TABLE[2 * STONES + 4 + base + op_gt - 1];
			begin = 0;
			end = index_end - index_begin + delta;
			gomoku_type_indice[base + op_gt] = gomoku_type_indice[base + op_gt - 1];
			action_indice[base + op_gt] = action_indice[base + op_gt - 1];
			rest_nb = 0;
			for (tmp_index = begin; tmp_index < end; tmp_index++)
			{
				act = tmp_types[tmp_index];
				func_idx = tmp_directions[tmp_index];
				key = get_fast_block_zobrisKey(_board, (int)act, player, (int)func_idx) ^ op_gt_key;
				/*if (block_coding_table.find(key) != block_coding_table.end())
				{
					continue;
				}
				
				block_coding_table.insert(key);*/
				if (GomokuTypeTable.in_table(key))
				{
					continue;
				}
				
				if (check_funcs[(op_gt - 1) / 2](_board, (int)act, color, move_list[(int)func_idx], 
					true, color == player, action_indice[base + op_gt], actions))
				{
					gomoku_types[gomoku_type_indice[base + op_gt]] = act;
					gomoku_directions[gomoku_type_indice[base + op_gt]++] = func_idx;
				}
				else
				{
					rest_actions_for_searching[rest_nb] = act;
					rest_directions_for_searching[rest_nb++] = func_idx;
				}
			}

			////////////////////////////////
			if (op_gt == OPEN_TWO)
			{
				continue;
			}

			index_begin = tmp_gomoku_type_indice[base + op_gt];
			index_end = tmp_gomoku_type_indice[base + op_gt + 1];

			if (color == opponent)
			{
				for (int j = 0; j < 4; j++)
				{
					tmp_types[j] = (unsigned char)action;
					tmp_directions[j] = (unsigned char)j;
				}
				delta = 4;
			}
			else
			{
				delta = 0;
			}

			memcpy(tmp_types + delta, tmp_gomoku_types + index_begin, (index_end - index_begin) * sizeof(unsigned char));
			memcpy(tmp_directions + delta, tmp_gomoku_directions + index_begin, (index_end - index_begin) * sizeof(unsigned char));

			memcpy(tmp_types + delta + index_end - index_begin, rest_actions_for_searching, rest_nb * sizeof(unsigned char));
			memcpy(tmp_directions + delta + index_end - index_begin, rest_directions_for_searching, rest_nb * sizeof(unsigned char));

			gt_key = FAST_ZOBRIST_TABLE[2 * STONES + 4 + base + op_gt];
			begin = 0;
			end = index_end - index_begin + delta +rest_nb;
			gomoku_type_indice[base + op_gt + 1] = gomoku_type_indice[base + op_gt];
			action_indice[base + op_gt + 1] = action_indice[base + op_gt];

			for (tmp_index = begin; tmp_index < end; tmp_index++)
			{
				act = tmp_types[tmp_index];
				func_idx = tmp_directions[tmp_index];
				key = get_fast_block_zobrisKey(_board, (int)act, player, (int)func_idx) ^ gt_key;

				/*if (block_coding_table.find(key) != block_coding_table.end())
				{
					continue;
				}

				block_coding_table.insert(key);*/
				if (GomokuTypeTable.in_table(key))
				{
					continue;
				}

				if (check_funcs[(op_gt - 1) / 2](_board, (int)act, color, move_list[(int)func_idx], 
					false, color == player, action_indice[base + op_gt + 1], actions))
				{
					gomoku_types[gomoku_type_indice[base + op_gt + 1]] = act;
					gomoku_directions[gomoku_type_indice[base + op_gt + 1]++] = func_idx;
				}

			}

		}

	}

	#ifdef FAST_DEBUG
	get_actions_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

}