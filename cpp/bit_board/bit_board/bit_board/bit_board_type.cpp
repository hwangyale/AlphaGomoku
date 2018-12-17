#include "bit_board.h"

extern Table TABLE;
extern GomokuTypeTable GOMOKU_TYPE_TABLE;
extern UCVEC SHARED_GOMOKU_TYPES;
extern UCVEC SHARED_DIRECTIONS;
extern UCVEC SHARED_ACTIONS;

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
	row += d;
	col -= d;
	return (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) ? -1 : row*BOARD_SIZE + col;
}

MOVE move_list[4] = { hor_move, ver_move, dia_move, bac_move };

bool check_single_action(BitBoard &board, UC action, UC direction, 
						 bool is_player, int gomoku_type,
						 UCVEC &container, int begin, int &count, 
						 std::bitset<STONES> &stored)
{
	int center, _count = 0, tmp_action;
	static int _container[BOARD_SIZE];
	GBIT &line = board.get_line(action, direction, center);
	GBIT &mask = TABLE.get_mask(action, direction);
	GBIT &masked_line = mask & line;

	bool flag = GOMOKU_TYPE_TABLE.get_actions(is_player, gomoku_type, masked_line, 
											  _container, 0, _count);

	MOVE &move_func = move_list[(int)direction];
	int index = 0;
	for (int i = 0; i < _count; i++)
	{
		tmp_action = move_func((int)action, _container[i] - center);
		if (stored[tmp_action] == 1)
		{
			continue;
		}
		container[begin + index++] = (UC)tmp_action;
		stored.set(tmp_action);
		count++;
	}

	return flag;
}

void set_continue_stones(BitBoard &board, int action, int direction, int color, 
						 std::bitset<STONES> &searched)
{
	int center, tmp_position, tmp_action;
	GBIT &line = board.get_line((UC)action, (UC)direction, center);
	GBIT &mask = TABLE.get_mask(action, direction);
	GBIT &masked_line = mask & line;
	MOVE &move_func = move_list[direction];
	for (int sign = -1; sign <= 1; sign += 2)
	{
		for (int d = 1; d <= 5; d++)
		{
			tmp_position = center + sign * d;
			if (tmp_position < 0 || tmp_position >= BOARD_SIZE)
			{
				break;
			}
			if (masked_line[2 * tmp_position] == 0) break;
			if (masked_line[2 * tmp_position + 1] != color) break;
			tmp_action = move_func(action, sign * d);
			searched.set(tmp_action);
		}
	}
}

void BitBoard::check_gomoku_type()
{
	UC action = history[history[0]];
	static int tmp_gomoku_indice[11];
	static UC gomoku_types[GOMOKU_TYPE_CONTAINER];
	static UC gomoku_directions[GOMOKU_TYPE_CONTAINER];
	static std::bitset<STONES> searched[4];
	static std::bitset<STONES> stored;
	static UC open_actions[GOMOKU_TYPE_CONTAINER];
	static UC open_directions[GOMOKU_TYPE_CONTAINER];
	static int open_count;
	static std::bitset<STONES> block_searched[4];
	
	memcpy(tmp_gomoku_indice, gomoku_indice, sizeof(tmp_gomoku_indice));
	for (int i = 0; i < gomoku_indice[10] - gomoku_indice[0]; i++)
	{
		gomoku_types[i] = SHARED_GOMOKU_TYPES[gomoku_indice[0] + i];
		gomoku_directions[i] = SHARED_DIRECTIONS[gomoku_indice[0] + i];
	}

	int opponent = player_mapping(player);
	int direction, index, end_index;
	UC tmp_action, tmp_direction;

	for (int color = BLACK; color <= WHITE; color++)
	{
		int base = color == BLACK ? 0 : 5;
		open_count = 0;
		for (int gt = OPEN_FOUR; gt <= OPEN_TWO; gt++)
		{
			stored.reset();
			int &gt_index = gomoku_indice[base + gt];
			int &act_index = action_indice[base + gt];
			gt_index = gomoku_indice[base + gt - 1];
			act_index = action_indice[base + gt - 1];

			if (color == opponent)
			{
				for (direction = 0; direction < 4; direction++)
				{
					searched[direction].reset();
					set_continue_stones(*this, (int)action, direction,
									    (int)(color == WHITE), searched[direction]);
					if (check_single_action(*this, (int)action, direction, false, gt,
										    SHARED_ACTIONS, act_index, act_index, stored))
					{
						SHARED_GOMOKU_TYPES[gt_index] = action;
						SHARED_DIRECTIONS[gt_index++] = (UC)direction;
					}
				}
			}

			if (gt % 2 == 0)
			{
				for (int block_direction = 0; block_direction < 4; block_direction++)
				{
					block_searched[block_direction].reset();
				}
			}

			end_index = tmp_gomoku_indice[base + gt] - tmp_gomoku_indice[0];
			for (index = tmp_gomoku_indice[base + gt - 1] - tmp_gomoku_indice[0];
				 index < end_index; index++)
			{
				tmp_action = gomoku_types[index];
				tmp_direction = gomoku_directions[index];
				if (color == opponent && searched[(int)tmp_direction][(int)tmp_action] == 1)
				{
					continue;
				}
				if (check_single_action(*this, (int)tmp_action, (int)tmp_direction,
									    color == player, gt,
										SHARED_ACTIONS, act_index, act_index, stored))
				{
					SHARED_GOMOKU_TYPES[gt_index] = tmp_action;
					SHARED_DIRECTIONS[gt_index++] = tmp_direction;
				}
				else if (gt % 2 == 1 && gt < OPEN_TWO)
				{
					open_actions[open_count] = tmp_action;
					open_directions[open_count++] = tmp_direction;
				}

				if (gt % 2 == 0)
				{
					block_searched[tmp_direction].set(tmp_action);
				}
			}

			if (gt % 2 == 0)
			{
				for (index = 0; index < open_count; index++)
				{
					tmp_action = open_actions[index];
					tmp_direction = open_directions[index];
					if (color == opponent && searched[(int)tmp_direction][(int)tmp_action] == 1)
					{
						continue;
					}
					if (block_searched[(int)tmp_direction][(int)tmp_action] == 1)
					{
						continue;
					}
					if (check_single_action(*this, (int)tmp_action, (int)tmp_direction,
											color == player, gt,
											SHARED_ACTIONS, act_index, act_index, stored))
					{
						SHARED_GOMOKU_TYPES[gt_index] = tmp_action;
						SHARED_DIRECTIONS[gt_index++] = tmp_direction;
					}
				}
				open_count = 0;
			}
		}
	}
}