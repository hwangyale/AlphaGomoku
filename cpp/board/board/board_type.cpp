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
#define FUNCTION_CODING 10000

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


#ifdef DEBUG

double check_time = 0.0;
double five_time = 0.0;
double four_time = 0.0;
double three_time = 0.0;
double two_time = 0.0;
double get_positions_time = 0.0;

#endif // DEBUG


void check(int cache_counts[], int cache_positions[], 
		   int _board[], int position, int color, 
		   MOVE move_func, int max_empty_count, int max_delta=6)
{
	clock_t start = clock();

	int empty_count, tmp_position, delta, opponent = player_mapping(color);
	std::fill_n(cache_counts, 2 * max_empty_count - 1, 0);
	std::fill_n(cache_positions, 2 * max_empty_count - 1, -1);
	cache_counts[max_empty_count - 1] = 1;
	cache_positions[max_empty_count - 1] = position;
	for (int sign = -1; sign <= 1; sign += 2)
	{
		empty_count = 0;
		for (delta = 1; delta < max_delta; delta++)
		{
			tmp_position = move_func(position, sign*delta);
			if (tmp_position < 0
				|| (_board[tmp_position] == opponent)
				|| empty_count == max_empty_count)
			{
				break;
			}
			if (_board[tmp_position] == color)
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
				cache_positions[sign*empty_count + max_empty_count - 1] = tmp_position;
			}
		}
	}


	#ifdef DEBUG
	check_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

}

std::unordered_map<std::string, std::vector<int>> GOMOKU_TYPE_TABLE;
std::string get_position_key(int gomoku_type, bool is_player, int cache_counts[], int cache_positions[], int max_empty_count)
{
	std::string key;
	std::vector<int>::iterator idx;

	key += std::to_string(gomoku_type);
	key += is_player ? '1' : '0';
	for (int idx = 0; idx < 2*max_empty_count-1; idx++)
	{
		key += std::to_string(cache_counts[idx]);
		key += cache_positions[idx] < 0 ? '0' : '1';
	}
	return key;
}


ISET CONTAINER;

bool check_five(int _board[], int position, int color, MOVE move_func)
{
	clock_t start = clock();

	int cache_counts[CHECK_SIZE], cache_positions[CHECK_SIZE];
	check(cache_counts, cache_positions, _board, position, color, move_func, 1);

	#ifdef DEBUG
	five_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

	if (cache_counts[0] >= 5)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool check_open_four_and_four(int _board[], int position, int color, MOVE move_func,
							  bool is_open, bool is_player, ISET &container = CONTAINER)
{
	clock_t start = clock();

	int index, max_empty_count = 3, cache_position, cache_counts[CHECK_SIZE], cache_positions[CHECK_SIZE];
	check(cache_counts, cache_positions, _board, position, color, move_func, max_empty_count, 7);

	std::string key = get_position_key((is_open ? OPEN_FOUR : FOUR), is_player, cache_counts, cache_positions, max_empty_count);

	if (GOMOKU_TYPE_TABLE.find(key) == GOMOKU_TYPE_TABLE.end())
	{
		int _indice[CHECK_SIZE] = { 0 };
		for (int i = 1; i < max_empty_count; i++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				index = sign * i + max_empty_count - 1;
				cache_position = cache_positions[index];
				if (cache_position < 0)
				{
					continue;
				}
				_board[cache_position] = color;
				if (check_five(_board, position, color, move_func))
				{
					_indice[++_indice[0]] = index;
				}
				_board[cache_position] = EMPTY;
			}
		}

		std::vector<int> indice;
		if ((is_open && _indice[0] >= 2) || (!is_open && _indice[0] == 1))
		{
			for (int idx = 1; idx <= _indice[0]; idx++)
			{
				indice.push_back(_indice[idx]);
			}
		}

		GOMOKU_TYPE_TABLE.insert(std::pair<std::string, std::vector<int>>(key, indice));
	}

	std::vector<int> &indice = GOMOKU_TYPE_TABLE[key];
	if (&container != &CONTAINER)
	{
		for (std::vector<int>::iterator idx = indice.begin(); idx != indice.end(); idx++)
		{
			container.insert(cache_positions[*idx]);
		}
	}

	#ifdef DEBUG
	four_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

	if (! indice.empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool check_open_three_and_three(int _board[], int position, int color, MOVE move_func,
							    bool is_open, bool is_player, ISET &container = CONTAINER)
{
	clock_t start = clock();

	int index, max_empty_count = 3, cache_position, cache_counts[CHECK_SIZE], cache_positions[CHECK_SIZE];
	check(cache_counts, cache_positions, _board, position, color, move_func, max_empty_count);

	std::string key = get_position_key((is_open ? OPEN_THREE : THREE), is_player, cache_counts, cache_positions, max_empty_count);

	if (GOMOKU_TYPE_TABLE.find(key) == GOMOKU_TYPE_TABLE.end())
	{
		std::vector<int> indice;
		int attack_position_indice[CHECK_SIZE] = { 0 };
		for (int i = 1; i < max_empty_count; i++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				index = sign * i + max_empty_count - 1;
				cache_position = cache_positions[index];
				if (cache_position < 0)
				{
					continue;
				}
				_board[cache_position] = color;
				if (check_open_four_and_four(_board, position, color, move_func, is_open, true))
				{
					if (is_player)
					{
						indice.push_back(index);
					}
					else
					{
						attack_position_indice[++attack_position_indice[0]] = index;
					}
				}
				_board[cache_position] = EMPTY;
			}
		}
		if (!is_player && attack_position_indice[0] > 0)
		{
			int player = player_mapping(color), _cache_position;
			bool flag;
			for (int i = 1; i < max_empty_count; i++)
			{
				for (int sign = -1; sign <= 1; sign += 2)
				{
					index = sign * i + max_empty_count - 1;
					cache_position = cache_positions[index];
					if (cache_position < 0)
					{
						continue;
					}
					_board[cache_position] = player;

					flag = true;
					for (int idx = 1; idx <= attack_position_indice[0]; idx++)
					{
						if (attack_position_indice[idx] == index)
						{
							continue;
						}
						_cache_position = cache_positions[attack_position_indice[idx]];
						_board[_cache_position] = color;
						if (check_open_four_and_four(_board, position, color, move_func, is_open, true))
						{
							_board[_cache_position] = EMPTY;
							_board[cache_position] = EMPTY;
							flag = false;
							break;
						}
						_board[_cache_position] = EMPTY;
					}
					if (flag)
					{
						_board[cache_position] = EMPTY;
						indice.push_back(index);
					}
				}
			}
		}

		GOMOKU_TYPE_TABLE.insert(std::pair<std::string, std::vector<int>>(key, indice));
	}

	std::vector<int> &indice = GOMOKU_TYPE_TABLE[key];
	if (&container != &CONTAINER)
	{
		for (std::vector<int>::iterator idx = indice.begin(); idx != indice.end(); idx++)
		{
			container.insert(cache_positions[*idx]);
		}
	}

	#ifdef DEBUG
		three_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

	if (! indice.empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool check_open_two_and_two(int _board[], int position, int color, MOVE move_func,
							bool is_open, bool is_player, ISET &container)
{
	clock_t start = clock();

	int index, max_empty_count = 4, cache_position, cache_counts[CHECK_SIZE], cache_positions[CHECK_SIZE];
	check(cache_counts, cache_positions, _board, position, color, move_func, max_empty_count);

	std::string key = get_position_key((is_open ? OPEN_TWO : TWO), is_player, cache_counts, cache_positions, max_empty_count);

	if (GOMOKU_TYPE_TABLE.find(key) == GOMOKU_TYPE_TABLE.end())
	{
		std::vector<int> indice;
		int attack_position_indice[CHECK_SIZE] = { 0 };
		for (int i = 1; i < max_empty_count; i++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				index = sign * i + max_empty_count - 1;
				cache_position = cache_positions[index];
				if (cache_position < 0)
				{
					continue;
				}
				_board[cache_position] = color;
				if (check_open_three_and_three(_board, position, color, move_func, is_open, true))
				{
					if (is_player)
					{
						indice.push_back(index);
					}
					else
					{
						attack_position_indice[++attack_position_indice[0]] = index;
					}
				}
				_board[cache_position] = EMPTY;
			}
		}

		if (!is_player && attack_position_indice[0] > 0)
		{
			int player = player_mapping(color), _cache_position;
			bool flag;
			for (int i = 1; i < max_empty_count; i++)
			{
				for (int sign = -1; sign <= 1; sign += 2)
				{
					index = sign * i + max_empty_count - 1;
					cache_position = cache_positions[index];
					if (cache_position < 0)
					{
						continue;
					}
					_board[cache_position] = player;

					flag = true;
					for (int idx = 1; idx <= attack_position_indice[0]; idx++)
					{
						if (attack_position_indice[idx] == index)
						{
							continue;
						}
						_cache_position = cache_positions[attack_position_indice[idx]];
						_board[_cache_position] = color;
						if (check_open_three_and_three(_board, position, color, move_func, is_open, true))
						{
							_board[_cache_position] = EMPTY;
							_board[cache_position] = EMPTY;
							flag = false;
							break;
						}
						_board[_cache_position] = EMPTY;
					}
					if (flag)
					{
						_board[cache_position] = EMPTY;
						indice.push_back(index);
					}
				}
			}
		}

		GOMOKU_TYPE_TABLE.insert(std::pair<std::string, std::vector<int>>(key, indice));
	}

	std::vector<int> &indice = GOMOKU_TYPE_TABLE[key];
	if (&container != &CONTAINER)
	{
		for (std::vector<int>::iterator idx = indice.begin(); idx != indice.end(); idx++)
		{
			container.insert(cache_positions[*idx]);
		}
	}

	#ifdef DEBUG
		two_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

	if (! indice.empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}


typedef bool(*CHECK)(int[], int, int, MOVE, bool, bool, ISET &);
CHECK check_funcs[3] = {check_open_four_and_four, check_open_three_and_three, check_open_two_and_two};


std::vector<U64> get_zobrist()
{
	std::vector<U64> _zobrist;
	U64 rand_number;
	srand(0);
	for (int i = 0; i < BOARD_SIZE*BOARD_SIZE * 2 + 4 + 12; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		_zobrist.push_back(rand_number);
	}
	return _zobrist;
}

std::vector<U64> ZOBRIST_TABLE = get_zobrist();

U64 get_block_zobrisKey(int _board[], int position, int color, int func_idx)
{
	U64 key = ZOBRIST_TABLE[position + (color == BLACK ? 0 : STONES)] ^ ZOBRIST_TABLE[2 * STONES + func_idx];
	MOVE move_func = move_list[func_idx];
	int tmp_position;
	for (int sign = -1; sign <= 1; sign += 2)
	{
		for (int delta = 1; delta < 6; delta++)
		{
			tmp_position = move_func(position, sign*delta);
			if (tmp_position >= 0 && _board[tmp_position] == color)
			{
				key ^= ZOBRIST_TABLE[tmp_position + (color == BLACK ? 0 : STONES)];
			}
			else
			{
				break;
			}
		}
	}
	return key;
}

void get_promising_positions(int _board[], ISET gomoku_types[2][6], ISET positions[12], 
							 int position, int player, bool check_two=true)
{
	clock_t start = clock();

	int opponent = player_mapping(player), tmp_pos, pos, func_idx;
	std::unordered_set<U64> block_coding_table;
	ISET::iterator set_it, set_end;
	U64 key;

	for (int color = BLACK; color <= WHITE; color++)
	{
		for (int gt = OPEN_FOUR; gt <= TWO; gt++)
		{
			if (color == opponent)
			{
				for (func_idx = 0; func_idx < 4; func_idx++)
				{
					gomoku_types[color - 1][gt - 1].insert(func_idx*FUNCTION_CODING + position);
				}
			}

		}

		for (int op_gt = OPEN_FOUR; op_gt <= OPEN_TWO; op_gt += 2)
		{
			U64 op_gt_key = ZOBRIST_TABLE[2 * STONES + 4 + (color == BLACK ? 0 : 6) + op_gt - 1];
			ISET rest_positions_for_searching;
			set_end = gomoku_types[color - 1][op_gt - 1].end();
			for (set_it = gomoku_types[color - 1][op_gt - 1].begin(); set_it != set_end; set_it++)
			{	
				tmp_pos = *set_it;
				func_idx = tmp_pos / FUNCTION_CODING;
				pos = tmp_pos % FUNCTION_CODING;
				key = get_block_zobrisKey(_board, pos, player, func_idx) ^ op_gt_key;

				if (block_coding_table.find(key) != block_coding_table.end())
				{
					rest_positions_for_searching.insert(tmp_pos);
					continue;
				}
				block_coding_table.insert(key);
				if (!check_funcs[(op_gt - 1) / 2](_board, pos, color, move_list[func_idx], true, color == player, 
												  positions[(color == player ? 0 : 6) + op_gt-1]))
				{
					rest_positions_for_searching.insert(tmp_pos);
				}
			}

			set_end = rest_positions_for_searching.end();
			for (set_it = rest_positions_for_searching.begin(); set_it != set_end; set_it++)
			{
				gomoku_types[color - 1][op_gt - 1].erase(*set_it);
			}

			if (op_gt == OPEN_TWO && !check_two)
			{
				continue;
			}
			rest_positions_for_searching.insert(gomoku_types[color - 1][op_gt].begin(), gomoku_types[color - 1][op_gt].end());
			U64 gt_key = ZOBRIST_TABLE[2 * STONES + 4 + (color == BLACK ? 0 : 6) + op_gt];
			set_end = rest_positions_for_searching.end();
			for (set_it = rest_positions_for_searching.begin(); set_it != set_end; set_it++)
			{
				tmp_pos = *set_it;
				func_idx = tmp_pos / FUNCTION_CODING;
				pos = tmp_pos % FUNCTION_CODING;
				key = get_block_zobrisKey(_board, pos, player, func_idx) ^ gt_key;

				if (block_coding_table.find(key) != block_coding_table.end())
				{
					gomoku_types[color - 1][op_gt].erase(tmp_pos);
					continue;
				}
				block_coding_table.insert(key);
				if (check_funcs[(op_gt - 1) / 2](_board, pos, color, move_list[func_idx], false, color == player, 
												 positions[(color == player ? 0 : 6) + op_gt]))
				{
					gomoku_types[color - 1][op_gt].insert(tmp_pos);
				}
				else
				{
					gomoku_types[color - 1][op_gt].erase(tmp_pos);
				}
			}
		}
	}

	#ifdef DEBUG
		get_positions_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // DEBUG

}