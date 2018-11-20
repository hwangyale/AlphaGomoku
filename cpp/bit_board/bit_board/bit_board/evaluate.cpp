#include "bit_board.h"

extern std::unordered_map<U64, int> BLACK_VCT_TABLE, WHITE_VCT_TABLE;

int BitBoard::evaluate(UC actions[], int begin_index, int &count, int current, 
					   std::unordered_map<U64, int> &cache_table, int unknown = -1)
{
	std::unordered_map<U64, int> &player_vct = current == BLACK ? BLACK_VCT_TABLE : WHITE_VCT_TABLE;
	std::unordered_map<U64, int> &opponent_vct = current != BLACK ? BLACK_VCT_TABLE : WHITE_VCT_TABLE;
	static UC tmp_actions[100];
	int tmp_count, tmp_idx;

	if (player_vct.find(zobristKey) != player_vct.end())
	{
		actions[begin_index] = player_vct[zobristKey];
		count++;
		return 1;
	}
	else if (opponent_vct.find(zobristKey) != opponent_vct.end())
	{
		return 0;
	}
	else if (cache_table.find(zobristKey) != cache_table.end() && cache_table[zobristKey])
	{
		return 0;
	}

	if (count_actions(true, OPEN_FOUR) > 0)
	{
		get_fast_actions(true, OPEN_FOUR, actions, begin_index, count);
		return current == player ? 1 : 0;
	}

	if (count_actions(true, FOUR) > 0)
	{
		get_fast_actions(true, FOUR, actions, begin_index, count);
		return current == player ? 1 : 0;
	}

	if (count_actions(false, OPEN_FOUR) > 0)
	{
		get_fast_actions(false, OPEN_FOUR, actions, begin_index, count);
		return current != player ? 1 : 0;
	}

	if (count_actions(false, FOUR) > 1)
	{
		get_fast_actions(false, FOUR, actions, begin_index, count);
		return current != player ? 1 : 0;
	}

	int value;
	if (count_actions(false, FOUR) == 1)
	{
		get_fast_actions(false, FOUR, actions, begin_index, count);
		if (current == player)
		{
			if (count_actions(true, OPEN_THREE) > 0)
			{
				value = unknown;
			}
			else
			{
				if (check_action(true, THREE, actions[begin_index]))
				{
					value = unknown;
				}
				else if (check_action(true, OPEN_TWO, actions[begin_index]))
				{
					value = unknown;
				}
				else
				{
					value = 0;
				}
			}
		}
		else
		{
			value = unknown;
		}
		return value;
	}

	if (count_actions(true, OPEN_THREE) > 0)
	{
		get_fast_actions(true, OPEN_THREE, actions, begin_index, count);
		return current == player ? 1 : 0;
	}

	tmp_count = 0;
	if (current == player)
	{
		if (count_actions(false, OPEN_THREE) > 0)
		{
			get_fast_actions(false, OPEN_THREE, tmp_actions, 0, tmp_count);
			tmp_idx = 0;
			for (int idx = 0; idx < tmp_count; idx++)
			{
				if (check_action(true, OPEN_TWO, tmp_actions[idx]))
				{
					tmp_actions[tmp_idx++] = tmp_actions[idx];
				}
			}
			tmp_count = tmp_idx;
		}
		else
		{
			get_fast_actions(true, OPEN_TWO, tmp_actions, 0, tmp_count);
		}

		get_fast_actions(true, THREE, tmp_actions, tmp_count, tmp_count);
		std::bitset<STONES> hash;
		tmp_idx = 0;
		for (int idx = 0; idx < tmp_count; idx++)
		{
			if (hash[tmp_actions[idx]] == 1)
			{
				continue;
			}
			hash.set(tmp_actions[idx]);
			actions[begin_index + tmp_idx++] = tmp_actions[idx];
			count++;
		}

		if (tmp_count > 0)
		{
			value = unknown;
		}
		else
		{
			value = 0;
		}
	}
	else
	{
		if (count_actions(false, OPEN_THREE) > 0)
		{
			get_fast_actions(true, THREE, tmp_actions, 0, tmp_count);
			get_fast_actions(false, OPEN_THREE, tmp_actions, tmp_count, tmp_count);

			std::bitset<STONES> hash;
			tmp_idx = 0;
			for (int idx = 0; idx < tmp_count; idx++)
			{
				if (hash[tmp_actions[idx]] == 1)
				{
					continue;
				}
				hash.set(tmp_actions[idx]);
				actions[begin_index + tmp_idx++] = tmp_actions[idx];
				count++;
			}

			value = unknown;
		}
		else
		{
			value = 0;
		}
	}
	return value;
}