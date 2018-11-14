#include "bit_board.h"
#define HASH_CONTAINER 300000

bool GomokuTypeTable::load()
{
	int todo;
}

bool GomokuTypeTable::save()
{
	int todo;
}

void check(int cache_counts[RANGE], int cache_actions[RANGE], GBIT &line, int max_empty_count, int center, int color)
{
	int empty_count, tmp_action, delta, opponent = player_mapping(color);
	for (int i = 0; i < RANGE; i++)
	{
		cache_counts[i] = 0;
		cache_actions[i] = -1;
	}
	cache_counts[max_empty_count - 1] = 1;
	cache_actions[max_empty_count - 1] = 1;
	for (int sign = -1; sign <= 1; sign += 2)
	{
		empty_count = 0;
		for (delta = 1; delta <= (RANGE - 1) / 2; delta++)
		{
			tmp_action = center + sign * delta;
			if (tmp_action < 0 || tmp_action >= BOARD_SIZE)
			{
				break;
			}
			if (empty_count == max_empty_count)
			{
				break;
			}
			if (line[2 * tmp_action] == 0 && line[2 * tmp_action + 1] == 0)
			{
				break;
			}
			if (line[2 * tmp_action] == 1 && line[2 * tmp_action + 1] != color)
			{
				break;
			}
			if (line[2 * tmp_action] == 1)
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
}

inline U64 rand_64()
{
	return rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
}

U64 get_key(int gomoku_type, bool is_player, int cache_counts[RANGE], int cache_actions[RANGE], int max_empty_count)
{
	static U64 gomokuZobristTable[5] = { 0 };
	static U64 playerZobristTable[2];
	static U64 countZobristTable[RANGE][7];
	static U64 actionZobristTable[RANGE];
	srand(0);
	if (gomokuZobristTable[0] == 0)
	{
		int i, j;
		for (i = 0; i < 5; i++)
		{
			gomokuZobristTable[i] = rand_64();
		}
		for (i = 0; i < 2; i++)
		{
			playerZobristTable[i] = rand_64();
		}
		for (i = 0; i < RANGE; i++)
		{
			for (j = 0; j < 7; j++)
			{
				countZobristTable[i][j] = rand_64();
			}
		}
		for (i = 0; i < RANGE; i++)
		{
			actionZobristTable[i] = rand_64();
		}
	}

	int delta, index;
	U64 key = 0;
	key ^= gomokuZobristTable[gomoku_type - 1];
	key ^= playerZobristTable[(int)is_player];
	key ^= countZobristTable[max_empty_count - 1][cache_counts[max_empty_count - 1]];
	key ^= actionZobristTable[max_empty_count - 1];

	for (int sign = -1; sign <= 1; sign += 2)
	{
		for (delta = 1; delta <= (RANGE - 1) / 2; delta++)
		{
			index = max_empty_count - 1 + sign * delta;
			if (index < 0 || index >= RANGE)
			{
				break;
			}
			if (cache_counts[index] == 0 && cache_actions[index] < 0)
			{
				break;
			}
			key ^= countZobristTable[index][cache_counts[index]];
			if (cache_actions[index] >= 0)
			{
				key ^= actionZobristTable[index];
			}
		}
	}
	return key;
}

int INSTRUCMENT_VALUE;

int cache_hash(U64 key, int &value = INSTRUCMENT_VALUE)
{
	static int hash_table[HASH_CONTAINER] = { 0 };
	static U64 key_table[HASH_CONTAINER];
	static int value_table[HASH_CONTAINER];
	static int next_table[HASH_CONTAINER];
	static int pointer_count = 1;

	int hash = (int)(key % HASH_CONTAINER);
	for (int pointer = hash_table[hash]; pointer != 0; pointer = next_table[pointer])
	{
		if (key_table[pointer] == key)
		{
			return value_table[pointer];
		}
	}

	if (&value == &INSTRUCMENT_VALUE)
	{
		return -1;
	}

	key_table[pointer_count] = key;
	value_table[pointer_count] = value;
	next_table[pointer_count] = hash_table[hash];
	hash_table[hash] = pointer_count++;
	return -1;
}

bool check_five(GBIT &line, int center, int color)
{
	static int cache_counts[RANGE], cache_actions[RANGE];
	check(cache_counts, cache_actions, line, 1, center, color);

	if (cache_counts[0] >= 5)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline void move(GBIT &line, int action, int color)
{
	line.set(2 * action);
	line.set(2 * action + 1, color);
}

inline void undo(GBIT &line, int action)
{
	line.set(2 * action, 0);
	line.set(2 * action + 1, 1);
}

int INSTRUCMENT_COUNT;

bool get_four_actions(GBIT &line, int center, int color, bool is_open, bool is_player,
					  UC container[RANGE + 1] = NULL, int begin = 0, int &count = INSTRUCMENT_COUNT)
{
	int index, max_empty_count = 3, cache_action, gomoku_type = is_open ? OPEN_FOUR : FOUR;
	static int cache_counts[RANGE], cache_actions[RANGE];
	static UC actions[BOARD_SIZE + 1];

	check(cache_counts, cache_actions, line, max_empty_count, center, color);
	U64 key = get_key(gomoku_type, is_player, cache_counts, cache_actions, max_empty_count);

	int hash_value = cache_hash(key);

	if (container == NULL && hash_value >= 0)
	{
		return (bool)hash_value;
	}

	int sign, delta;
	actions[0] = 0;
	for (sign = -1; sign <= 1; sign += 2)
	{
		for (delta = 1; delta <= (RANGE - 1) / 2; delta++)
		{
			index = max_empty_count - 1 + sign * delta;
			if (index < 0 || index >= RANGE)
			{
				break;
			}
			cache_action = cache_actions[index];
			if (cache_action < 0)
			{
				break;
			}
			move(line, cache_action, color);
			if (check_five(line, cache_action, color))
			{
				actions[++actions[0]] = (UC)cache_action;
			}
			undo(line, cache_action);
		}
	}

	if ((is_open && actions[0] >= 2) || (!is_open && actions[0] == 1))
	{
		hash_value = 1;
		if (container != NULL)
		{
			for (int i = 0; i < actions[0]; i++)
			{
				container[begin + i] = actions[i + 1];
				count++;
			}
		}
	}
	else
	{
		hash_value = 0;
	}

	cache_hash(key, hash_value);
	return (bool)hash_value;
}

bool get_three_actions(GBIT &line, int center, int color, bool is_open, bool is_player,
				       UC container[RANGE + 1] = NULL, int begin = 0, int &count = INSTRUCMENT_COUNT)
{

}

bool get_two_actions(GBIT &line, int center, int color, bool is_open, bool is_player, 
	                 UC container[RANGE + 1] = NULL, int begin = 0, int &count = INSTRUCMENT_COUNT)
{
	
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
	bool is_open, bool is_player, int &begin = BEGIN, unsigned char *actions = NULL)
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