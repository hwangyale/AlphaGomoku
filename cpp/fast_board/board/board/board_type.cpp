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

inline int hor_move(int action, int d)
{
	int col = action % BOARD_SIZE + d;
	return (col >= 0 && col < BOARD_SIZE) ? action + d : -1;
}

inline int ver_move(int action, int d)
{
	int new_action = action + d * BOARD_SIZE;
	return (new_action >= 0 && new_action < STONES) ? new_action : -1;
}

inline int dia_move(int action, int d)
{
	int row = action / BOARD_SIZE, col = action % BOARD_SIZE;
	row += d;
	col += d;
	return (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) ? -1 : row*BOARD_SIZE + col;
}

inline int bac_move(int action, int d)
{
	int row = action / BOARD_SIZE, col = action % BOARD_SIZE;
	row -= d;
	col += d;
	return (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) ? -1 : row*BOARD_SIZE + col;
}

MOVE move_list[4] = { hor_move, ver_move, dia_move, bac_move };


inline int player_mapping(int player)
{
	return player == BLACK ? WHITE : BLACK;
}

#ifdef FAST_DEBUG

double check_time = 0.0;
double five_time = 0.0;
double four_time = 0.0;
double three_time = 0.0;
double two_time = 0.0;
double get_actions_time = 0.0;

#endif // FAST_DEBUG

///////////////////////////GomokuTypeHash//////////////////////
class GomokuTypeHash
{
public:
	bool in_table(U64 code);
	void reset();
	GomokuTypeHash();
private:
	U64 values[FASTBOARD_CONTAINER];
	int next[FASTBOARD_CONTAINER];
	int heads[GOMOKU_TYPE_CONTAINER];
	int head_stack[FASTBOARD_CONTAINER];
	int hashed_number, pointer, hash;
};

GomokuTypeHash::GomokuTypeHash()
{
	memset(values, 0, sizeof(values));
	memset(next, 0, sizeof(next));
	memset(heads, 0, sizeof(heads));
	memset(head_stack, 0, sizeof(head_stack));
	hashed_number = 1;
}

void GomokuTypeHash::reset()
{
	while (hashed_number > 1)
	{
		heads[head_stack[--hashed_number]] = 0;
	}
}

bool GomokuTypeHash::in_table(U64 code)
{
	hash = (int)(code % GOMOKU_TYPE_CONTAINER);
	for (pointer = heads[hash]; pointer != 0; pointer = next[pointer])
	{
		if (values[pointer] == code)
		{
			return true;
		}
	}

	values[hashed_number] = code;
	next[hashed_number] = heads[hash];
	heads[hash] = hashed_number;
	head_stack[hashed_number++] = hash;
	return false;
}

///////////////////////////ActionHash//////////////////////////

class ActionHash
{
public:
	bool check(int gt, int action, int color);
	void reset();
	ActionHash();
private:
	int action_counts[2][5];
	int action_stacks[2][5][STONES + 1];
	bool actions[2][5][STONES];
};

ActionHash::ActionHash()
{
	memset(action_counts[0], 0, sizeof(action_counts[0]));
	memset(action_counts[1], 0, sizeof(action_counts[1]));
	for (int i = 0; i < 5; i++)
	{
		memset(action_stacks[0][i], 0, sizeof(action_stacks[0][i]));
		memset(action_stacks[1][i], 0, sizeof(action_stacks[1][i]));
		memset(actions[0][i], 0, sizeof(actions[0][i]));
		memset(actions[1][i], 0, sizeof(actions[1][i]));
	}
}

void ActionHash::reset()
{
	for (int i = 0; i < 5; i++)
	{
		while (action_counts[0][i] > 0)
		{
			actions[0][i][action_stacks[0][i][--action_counts[0][i]]] = false;
		}
		while (action_counts[1][i] > 0)
		{
			actions[1][i][action_stacks[1][i][--action_counts[1][i]]] = false;
		}
	}
}

bool ActionHash::check(int gt, int action, int color)
{
	bool in_table = actions[color - 1][gt - 1][action];
	if (!in_table)
	{
		action_stacks[color - 1][gt - 1][action_counts[color - 1][gt - 1]++] = action;
		actions[color - 1][gt - 1][action] = true;
	}
	return in_table;
}


///////////////////////////Board Type//////////////////////////

void fast_check(int cache_counts[], int cache_actions[],
			    int _board[], int action, int color,
				MOVE move_func, int max_empty_count, int max_delta = 6)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif

	int empty_count, tmp_action, delta, opponent = player_mapping(color);
	for (int tmp_i = 0; tmp_i < CHECK_SIZE; tmp_i++) cache_counts[tmp_i] = 0;
	std::fill_n(cache_actions, CHECK_SIZE, -1);
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

std::unordered_map<U64, int> GOMOKU_TYPE_HEAD_TABLE;
int GOMOKU_TYPE_TABLE[1000000] = { 0 }, GOMOKU_TYPE_NEXT_TABLE[1000000] = { 0 }, GOMOKU_TYPE_TABLE_INDEX = 1;

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

	if (GOMOKU_TYPE_HEAD_TABLE.find(key) == GOMOKU_TYPE_HEAD_TABLE.end())
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

		GOMOKU_TYPE_HEAD_TABLE[key] = 0;
		int &head = GOMOKU_TYPE_HEAD_TABLE[key];
		if ((is_open && _indice[0] >= 2) || (!is_open && _indice[0] == 1))
		{
			for (int idx = 1; idx <= _indice[0]; idx++)
			{
				GOMOKU_TYPE_TABLE[GOMOKU_TYPE_TABLE_INDEX] = _indice[idx];
				GOMOKU_TYPE_NEXT_TABLE[GOMOKU_TYPE_TABLE_INDEX] = head;
				head = GOMOKU_TYPE_TABLE_INDEX++;
			}
		}
	}

	int head_pointer = GOMOKU_TYPE_HEAD_TABLE[key];
	if (actions != NULL && head_pointer != 0)
	{
		int check_gt = is_open ? OPEN_FOUR : FOUR;
		int act_idx;
		for (int gomoku_pointer = head_pointer; gomoku_pointer != 0; gomoku_pointer = GOMOKU_TYPE_NEXT_TABLE[gomoku_pointer])
		{
			act_idx = GOMOKU_TYPE_TABLE[gomoku_pointer];
			if (!ActionTable.check(check_gt, cache_actions[act_idx], color))
			{
				actions[begin++] = (unsigned char)cache_actions[act_idx];
			}
		}
	}

	#ifdef FAST_DEBUG
	four_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (head_pointer != 0)
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
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif

	int index, max_empty_count = 3, cache_action, cache_counts[CHECK_SIZE], cache_actions[CHECK_SIZE];
	fast_check(cache_counts, cache_actions, _board, action, color, move_func, max_empty_count);

	U64 key = get_action_key((is_open ? OPEN_THREE : THREE), is_player, cache_counts, cache_actions, max_empty_count);

	if (GOMOKU_TYPE_HEAD_TABLE.find(key) == GOMOKU_TYPE_HEAD_TABLE.end())
	{
		GOMOKU_TYPE_HEAD_TABLE[key] = 0;
		int &head = GOMOKU_TYPE_HEAD_TABLE[key];
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
						GOMOKU_TYPE_TABLE[GOMOKU_TYPE_TABLE_INDEX] = index;
						GOMOKU_TYPE_NEXT_TABLE[GOMOKU_TYPE_TABLE_INDEX] = head;
						head = GOMOKU_TYPE_TABLE_INDEX++;
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
						GOMOKU_TYPE_TABLE[GOMOKU_TYPE_TABLE_INDEX] = index;
						GOMOKU_TYPE_NEXT_TABLE[GOMOKU_TYPE_TABLE_INDEX] = head;
						head = GOMOKU_TYPE_TABLE_INDEX++;
					}
				}
			}
		}
	}

	int head_pointer = GOMOKU_TYPE_HEAD_TABLE[key];
	if (actions != NULL && head_pointer != 0)
	{
		int check_gt = is_open ? OPEN_THREE : THREE;
		int act_idx;
		for (int gomoku_pointer = head_pointer; gomoku_pointer != 0; gomoku_pointer = GOMOKU_TYPE_NEXT_TABLE[gomoku_pointer])
		{
			act_idx = GOMOKU_TYPE_TABLE[gomoku_pointer];
			if (!ActionTable.check(check_gt, cache_actions[act_idx], color))
			{
				actions[begin++] = (unsigned char)cache_actions[act_idx];
			}
		}
	}

	#ifdef FAST_DEBUG
	three_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (head_pointer != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool check_fast_open_two(int _board[], int action, int color, MOVE move_func,
	bool is_open, bool is_player, int &begin = BEGIN, unsigned char *actions = NULL)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();

	#endif // FAST_DEBUG

	int index, max_empty_count = 4, cache_action, cache_counts[CHECK_SIZE], cache_actions[CHECK_SIZE];
	fast_check(cache_counts, cache_actions, _board, action, color, move_func, max_empty_count);

	U64 key = get_action_key(OPEN_TWO, is_player, cache_counts, cache_actions, max_empty_count);

	if (GOMOKU_TYPE_HEAD_TABLE.find(key) == GOMOKU_TYPE_HEAD_TABLE.end())
	{
		GOMOKU_TYPE_HEAD_TABLE[key] = 0;
		int &head = GOMOKU_TYPE_HEAD_TABLE[key];
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
						GOMOKU_TYPE_TABLE[GOMOKU_TYPE_TABLE_INDEX] = index;
						GOMOKU_TYPE_NEXT_TABLE[GOMOKU_TYPE_TABLE_INDEX] = head;
						head = GOMOKU_TYPE_TABLE_INDEX++;
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
						GOMOKU_TYPE_TABLE[GOMOKU_TYPE_TABLE_INDEX] = index;
						GOMOKU_TYPE_NEXT_TABLE[GOMOKU_TYPE_TABLE_INDEX] = head;
						head = GOMOKU_TYPE_TABLE_INDEX++;
					}
				}
			}
		}
	}

	int head_pointer = GOMOKU_TYPE_HEAD_TABLE[key];
	if (actions != NULL && head_pointer != 0)
	{
		int check_gt = OPEN_TWO;
		int act_idx;
		for (int gomoku_pointer = head_pointer; gomoku_pointer != 0; gomoku_pointer = GOMOKU_TYPE_NEXT_TABLE[gomoku_pointer])
		{
			act_idx = GOMOKU_TYPE_TABLE[gomoku_pointer];
			if (!ActionTable.check(check_gt, cache_actions[act_idx], color))
			{
				actions[begin++] = (unsigned char)cache_actions[act_idx];
			}
		}
	}

	#ifdef FAST_DEBUG
	two_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

	if (head_pointer != 0)
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

class ExpandedZobristTable
{
public:
	U64 keys[2 * STONES + 4 + 10];

	ExpandedZobristTable();
	inline U64 get_key(int color, int action);
};

ExpandedZobristTable::ExpandedZobristTable()
{
	U64 rand_number;
	srand(0);
	for (int i = 0; i < 2 * STONES + 4 + 10; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		keys[i] = rand_number;
	}
}

inline U64 ExpandedZobristTable::get_key(int color, int action)
{
	return keys[2 * action + color - 1];
}

ExpandedZobristTable zobristTable = ExpandedZobristTable();

U64 get_fast_block_zobrisKey(int _board[], int action, int color, int func_idx)
{
	U64 key = zobristTable.get_key(color, action) ^ zobristTable.keys[2 * STONES + func_idx];
	MOVE move_func = move_list[func_idx];
	int tmp_action;
	for (int sign = -1; sign <= 1; sign += 2)
	{
		for (int delta = 1; delta < 6; delta++)
		{
			tmp_action = move_func(action, sign*delta);
			if (tmp_action >= 0 && _board[tmp_action] == color)
			{
				key ^= zobristTable.get_key(color, tmp_action);
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

			op_gt_key = zobristTable.keys[2 * STONES + 4 + base + op_gt - 1];
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

			gt_key = zobristTable.keys[2 * STONES + 4 + base + op_gt];
			begin = 0;
			end = index_end - index_begin + delta +rest_nb;
			gomoku_type_indice[base + op_gt + 1] = gomoku_type_indice[base + op_gt];
			action_indice[base + op_gt + 1] = action_indice[base + op_gt];

			for (tmp_index = begin; tmp_index < end; tmp_index++)
			{
				act = tmp_types[tmp_index];
				func_idx = tmp_directions[tmp_index];
				key = get_fast_block_zobrisKey(_board, (int)act, player, (int)func_idx) ^ gt_key;

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