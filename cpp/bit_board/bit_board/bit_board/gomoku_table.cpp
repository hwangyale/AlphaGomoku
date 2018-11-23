#include "bit_board.h"
#define HASH_CONTAINER 300000
#define GOMOKU_TYPE_HASH_TABLE_CONTAINER 500000

int HASH_TABLE[2][5][GOMOKU_TYPE_HASH_TABLE_CONTAINER];
U64 KEY_TABLE[2][5][GOMOKU_TYPE_HASH_TABLE_CONTAINER];
int HEAD_TABLE[2][5][GOMOKU_TYPE_HASH_TABLE_CONTAINER];
int HEAD_NEXT_TABLE[2][5][GOMOKU_TYPE_HASH_TABLE_CONTAINER];
int ACTION_TABLE[2][5][GOMOKU_TYPE_HASH_TABLE_CONTAINER * 4];
int ACTION_NEXT_TABLE[2][5][GOMOKU_TYPE_HASH_TABLE_CONTAINER * 4];
int gomoku_pointers[2][5], action_pointers[2][5], hash_counts[2][5];

GomokuTypeTable GOMOKU_TYPE_TABLE;

void check(int cache_counts[RANGE], int cache_actions[RANGE], GBIT &line, int max_empty_count, int center, bool color)
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

int INSTRintMENT_VALUE;

int cache_hash(U64 key, int &value = INSTRintMENT_VALUE)
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

	if (&value == &INSTRintMENT_VALUE)
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
					  int container[RANGE + 1] = NULL, int begin = 0, int &count = INSTRUCMENT_COUNT)
{
	int index, max_empty_count = 3, cache_action, gomoku_type = is_open ? OPEN_FOUR : FOUR;
	static int cache_counts[RANGE], cache_actions[RANGE];
	static int actions[BOARD_SIZE + 1];

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
				actions[++actions[0]] = (int)cache_action;
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
				container[begin + i] = (int)actions[i + 1];
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
				       int container[RANGE + 1] = NULL, int begin = 0, int &count = INSTRUCMENT_COUNT)
{
	int index, max_empty_count = 3, cache_action, gomoku_type = is_open ? OPEN_THREE : THREE;
	static int cache_counts[RANGE], cache_actions[RANGE];
	static int actions[BOARD_SIZE + 1], attack_actions[BOARD_SIZE + 1];

	check(cache_counts, cache_actions, line, max_empty_count, center, color);
	U64 key = get_key(gomoku_type, is_player, cache_counts, cache_actions, max_empty_count);

	int hash_value = cache_hash(key);

	if (container == NULL && hash_value >= 0)
	{
		return (bool)hash_value;
	}

	int sign, delta;
	actions[0] = 0;
	attack_actions[0] = 0;
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
			if (get_four_actions(line, center, color, is_open, true))
			{
				if (is_player)
				{
					actions[++actions[0]] = (int)cache_action;
				}
				else
				{
					attack_actions[++attack_actions[0]] = (int)cache_action;
				}
			}
			undo(line, cache_action);
		}
	}

	hash_value = actions[0] > 0 || attack_actions[0] > 0;
	cache_hash(key, hash_value);

	if (!is_player && attack_actions[0] > 0)
	{
		int player = color ^ 1, _cache_action, idx;
		bool flag;
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
				move(line, cache_action, player);

				flag = true;
				for (idx = 1; idx <= attack_actions[0]; idx++)
				{
					if (attack_actions[idx] == cache_action)
					{
						continue;
					}
					_cache_action = attack_actions[idx];
					move(line, _cache_action, color);
					if (get_four_actions(line, center, color, is_open, true))
					{
						undo(line, _cache_action);
						undo(line, cache_action);
						flag = false;
						break;
					}
					undo(line, _cache_action);
				}
				if (flag)
				{
					undo(line, cache_action);
					actions[++actions[0]] = cache_action;
				}
			}
		}
	}

	if (container != NULL)
	{
		for (int i = 0; i < actions[0]; i++)
		{
			container[begin + i] = (int)actions[i + 1];
			count++;
		}
	}

	return (bool)hash_value;
}


bool get_two_actions(GBIT &line, int center, int color, bool is_open, bool is_player, 
	                 int container[RANGE + 1] = NULL, int begin = 0, int &count = INSTRUCMENT_COUNT)
{
	int index, max_empty_count = 4, cache_action, gomoku_type = OPEN_TWO;
	static int cache_counts[RANGE], cache_actions[RANGE];
	static int actions[BOARD_SIZE + 1], attack_actions[BOARD_SIZE + 1];

	check(cache_counts, cache_actions, line, max_empty_count, center, color);
	U64 key = get_key(gomoku_type, is_player, cache_counts, cache_actions, max_empty_count);

	int hash_value = cache_hash(key);

	if (container == NULL && hash_value >= 0)
	{
		return (bool)hash_value;
	}

	int sign, delta;
	actions[0] = 0;
	attack_actions[0] = 0;
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
			if (get_three_actions(line, center, color, true, true))
			{
				if (is_player)
				{
					actions[++actions[0]] = (int)cache_action;
				}
				else
				{
					attack_actions[++attack_actions[0]] = (int)cache_action;
				}
			}
			undo(line, cache_action);
		}
	}

	hash_value = actions[0] > 0 || attack_actions[0] > 0;
	cache_hash(key, hash_value);

	if (!is_player && attack_actions[0] > 0)
	{
		int player = color ^ 1, _cache_action, idx;
		bool flag;
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
				move(line, cache_action, player);

				flag = true;
				for (idx = 1; idx <= attack_actions[0]; idx++)
				{
					if (attack_actions[idx] == cache_action)
					{
						continue;
					}
					_cache_action = attack_actions[idx];
					move(line, _cache_action, color);
					if (get_three_actions(line, center, color, true, true))
					{
						undo(line, _cache_action);
						undo(line, cache_action);
						flag = false;
						break;
					}
					undo(line, _cache_action);
				}
				if (flag)
				{
					undo(line, cache_action);
					actions[++actions[0]] = cache_action;
				}
			}
		}
	}

	if (container != NULL)
	{
		for (int i = 0; i < actions[0]; i++)
		{
			container[begin + i] = (int)actions[i + 1];
			count++;
		}
	}

	return (bool)hash_value;
}


typedef bool(*GetActions)(GBIT &, int, int, bool, bool, int *, int, int &);
GetActions get_action_functions[3] = { get_four_actions, get_three_actions, get_two_actions };

void generate_actions(int gomoku_type, bool is_player,
					  int hash_table[GOMOKU_TYPE_HASH_TABLE_CONTAINER],
					  U64 key_table[GOMOKU_TYPE_HASH_TABLE_CONTAINER],
					  int head_table[GOMOKU_TYPE_HASH_TABLE_CONTAINER],
					  int head_next_table[GOMOKU_TYPE_HASH_TABLE_CONTAINER],
					  int action_table[GOMOKU_TYPE_HASH_TABLE_CONTAINER * 4],
					  int action_next_table[GOMOKU_TYPE_HASH_TABLE_CONTAINER * 4], 
					  int &gomoku_pointer, int &action_pointer, int &hash_count)
{
	int tmp_gomoku_pointer, action_head, action_index;
	int i, begin_index, end_index, center, count;
	int tmp_min, tmp_max, state, state_index, tmp_state_index, color, hash;
	int action_container[BOARD_SIZE + 1];
	U64 key;
	GBIT line;

	bool is_open = gomoku_type % 2 == 1;
	GetActions get_action_func = get_action_functions[(gomoku_type - 1) / 2];

	gomoku_pointer = 1;
	action_pointer = 1;
	hash_count = 0;

	for (i = 0; i < GOMOKU_TYPE_HASH_TABLE_CONTAINER; i++)
	{
		hash_table[i] = 0;
	}

	for (int size = 1; size <= BOARD_SIZE; size++)
	{
		begin_index = size <= HALF_SIZE ? 0 : BOARD_SIZE - size;
		end_index = size <= HALF_SIZE ? size : BOARD_SIZE;
		for (center = begin_index; center < end_index; center++)
		{
			count = 1;
			tmp_min = MAX(begin_index, center - (RANGE - 1) / 2);
			tmp_max = MIN(end_index, center + (RANGE - 1) / 2 + 1);
			for (i = tmp_min; i < tmp_max; i++)
			{
				count *= 3;
			}
			for (state_index = 0; state_index < count; state_index++)
			{
				line.reset();
				tmp_state_index = state_index;
				for (i = tmp_min; i < tmp_max; i++)
				{
					state = tmp_state_index % 3;
					tmp_state_index /= 3;
					switch (state)
					{
					case 0:
						line.set(2 * i + 1);
						break;
					case 1:
						line.set(2 * i);
						break;
					default:
						line.set(2 * i);
						line.set(2 * i + 1);
						break;
					}
				}
				if (line[2 * center] == 0)
				{
					continue;
				}
				std::cout << is_player << " " << gomoku_type << " " << line << std::endl;
				color = line[2 * center + 1];
				action_container[0] = 0;
				get_action_func(line, center, color, is_open, is_player,
								action_container, 1, action_container[0]);
				if (action_container[0] > 0)
				{
					key = line.to_ullong();
					hash = (int)(key % GOMOKU_TYPE_HASH_TABLE_CONTAINER);
					action_head = -1;
					for (tmp_gomoku_pointer = hash_table[hash]; tmp_gomoku_pointer != 0;
						 tmp_gomoku_pointer = head_next_table[tmp_gomoku_pointer])
					{
						if (key_table[tmp_gomoku_pointer] == key)
						{
							action_head = head_table[tmp_gomoku_pointer];
							break;
						}
					}
					if (action_head < 0)
					{
						action_head = 0;
						for (action_index = 1; action_index <= action_container[0]; action_index++)
						{
							action_table[action_pointer] = action_container[action_index];
							action_next_table[action_pointer] = action_head;
							action_head = action_pointer++;
						}

						key_table[gomoku_pointer] = key;
						head_table[gomoku_pointer] = action_head;
						head_next_table[gomoku_pointer] = hash_table[hash];
						if (hash_table[hash] == 0)
						{
							hash_count++;
						}
						hash_table[hash] = gomoku_pointer++;
					}
				}
			}
		}
	}
}

void GomokuTypeTable::generate_action_hash_table()
{
	for (int is_player = 0; is_player <= 1; is_player++)
	{
		for (int gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			generate_actions(gomoku_type, (bool)is_player,
							 HASH_TABLE[is_player][gomoku_type - 1],
							 KEY_TABLE[is_player][gomoku_type - 1], 
							 HEAD_TABLE[is_player][gomoku_type - 1], 
							 HEAD_NEXT_TABLE[is_player][gomoku_type - 1], 
						     ACTION_TABLE[is_player][gomoku_type - 1], 
							 ACTION_NEXT_TABLE[is_player][gomoku_type - 1], 
							 gomoku_pointers[is_player][gomoku_type - 1], 
							 action_pointers[is_player][gomoku_type - 1], 
							 hash_counts[is_player][gomoku_type - 1]);
		}
	}
}

bool GomokuTypeTable::load()
{
	if (freopen("gomoku_hash_table.txt", "r", stdin) == NULL)
	{
		return false;
	}

	int is_player, gomoku_type, count, i, tmp_i, tmp_hash;
	for (is_player = 0; is_player <= 1; is_player++)
	{
		for (gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			scanf("%d", &hash_counts[is_player][gomoku_type - 1]);
			count = 0;
			for (i = 0; i < hash_counts[is_player][gomoku_type - 1]; i++)
			{
				scanf("%d %d", &tmp_i, &tmp_hash);
				HASH_TABLE[is_player][gomoku_type - 1][tmp_i] = tmp_hash;
			}
		}
	}
	for (is_player = 0; is_player <= 1; is_player++)
	{
		for (gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			scanf("%d", &gomoku_pointers[is_player][gomoku_type - 1]);
			for (i = 1; i < gomoku_pointers[is_player][gomoku_type - 1]; i++)
			{
				scanf("%llu %d %d",
					  &KEY_TABLE[is_player][gomoku_type - 1][i],
					  &HEAD_TABLE[is_player][gomoku_type - 1][i],
					  &HEAD_NEXT_TABLE[is_player][gomoku_type - 1][i]);
			}
		}
	}
	for (is_player = 0; is_player <= 1; is_player++)
	{
		for (gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			scanf("%d", &action_pointers[is_player][gomoku_type - 1]);
			for (i = 1; i < action_pointers[is_player][gomoku_type - 1]; i++)
			{
				scanf("%d %d",
					  &ACTION_TABLE[is_player][gomoku_type - 1][i],
					  &ACTION_NEXT_TABLE[is_player][gomoku_type - 1][i]);
			}
		}
	}
	fclose(stdin);
	return true;
}

bool GomokuTypeTable::save()
{
	int is_player, gomoku_type, count, i;
	freopen("gomoku_hash_table.txt", "w", stdout);
	for (is_player = 0; is_player <= 1; is_player++)
	{
		for (gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			printf("%d\n", hash_counts[is_player][gomoku_type - 1]);
			count = 0;
			for (i = 0; i < GOMOKU_TYPE_HASH_TABLE_CONTAINER; i++)
			{
				if (HASH_TABLE[is_player][gomoku_type - 1][i] == 0)
				{
					continue;
				}
				printf("%d %d\n", i, HASH_TABLE[is_player][gomoku_type - 1][i]);
				count++;
				if (count == hash_counts[is_player][gomoku_type - 1])
				{
					break;
				}
			}
		}
	}
	for (is_player = 0; is_player <= 1; is_player++)
	{
		for (gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			printf("%d\n", gomoku_pointers[is_player][gomoku_type - 1]);
			for (i = 1; i < gomoku_pointers[is_player][gomoku_type - 1]; i++)
			{
				printf("%llu %d %d\n", 
					   KEY_TABLE[is_player][gomoku_type - 1][i],
					   HEAD_TABLE[is_player][gomoku_type - 1][i],
					   HEAD_NEXT_TABLE[is_player][gomoku_type - 1][i]);
			}
		}
	}
	for (is_player = 0; is_player <= 1; is_player++)
	{
		for (gomoku_type = OPEN_FOUR; gomoku_type <= OPEN_TWO; gomoku_type++)
		{
			printf("%d\n", action_pointers[is_player][gomoku_type - 1]);
			for (i = 1; i < action_pointers[is_player][gomoku_type - 1]; i++)
			{
				printf("%d %d\n",
					   ACTION_TABLE[is_player][gomoku_type - 1][i],
					   ACTION_NEXT_TABLE[is_player][gomoku_type - 1][i]);
			}
		}
	}
	fclose(stdout);
	return true;
}

GomokuTypeTable::GomokuTypeTable()
{
	if (!load())
	{
		generate_action_hash_table();
		save();
	}
}

bool GomokuTypeTable::get_actions(bool is_player, int gomoku_type, GBIT &masked_line, int container[], int begin, int &count)
{
	U64 key = masked_line.to_ullong();
	int hash = (int)(key % GOMOKU_TYPE_HASH_TABLE_CONTAINER);
	int index = 0, gt_pointer, act_pointer = -1;
	static int ins_container[1], ins_begin, ins_count;
	for (gt_pointer = HASH_TABLE[(int)is_player][gomoku_type - 1][hash]; gt_pointer != 0;
		 gt_pointer = HEAD_NEXT_TABLE[(int)is_player][gomoku_type - 1][gt_pointer])
	{
		if (KEY_TABLE[(int)is_player][gomoku_type - 1][gt_pointer] == key)
		{
			act_pointer = HEAD_TABLE[(int)is_player][gomoku_type - 1][gt_pointer];
			break;
		}
	}
	if (act_pointer < 0)
	{
		if (is_player)
		{
			return false;
		}
		return get_actions(true, gomoku_type, masked_line, ins_container, ins_begin, ins_count);
	}
	while (act_pointer != 0)
	{
		container[begin + index++] = ACTION_TABLE[(int)is_player][gomoku_type - 1][act_pointer];
		count++;
		act_pointer = ACTION_NEXT_TABLE[(int)is_player][gomoku_type - 1][act_pointer];
	}
	return true;
}

