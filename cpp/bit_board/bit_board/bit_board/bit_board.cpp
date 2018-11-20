#include "bit_board.h"

UC SHARED_GOMOKU_TYPES[MAX_BOARD * GOMOKU_TYPE_CONTAINER] = { 0 };
UC SHARED_DIRECTIONS[MAX_BOARD * GOMOKU_TYPE_CONTAINER] = { 0 };
UC SHARED_ACTIONS[MAX_BOARD * ACTION_CONTAINER] = { 0 };
int SHARED_INDEX = 0;
int SHARED_INDEX_QUEUE[MAX_BOARD] = { 0 }, SHARED_INDEX_HEAD = 0, SHARED_INDEX_TAIL = 0;

extern Table TABLE;

void BitBoard::allocate()
{
	if (!allocated)
	{
		int shared_index;
		if (SHARED_INDEX_HEAD != SHARED_INDEX_TAIL)
		{
			shared_index = SHARED_INDEX_QUEUE[SHARED_INDEX_HEAD];
			SHARED_INDEX_HEAD = (SHARED_INDEX_HEAD + 1) % MAX_BOARD;
		}
		else
		{
			shared_index = SHARED_INDEX;
			SHARED_INDEX++;
		}
		gomoku_indice[0] = shared_index * GOMOKU_TYPE_CONTAINER;
		action_indice[0] = shared_index * ACTION_CONTAINER;
		allocated = true;
	}
}

void BitBoard::release()
{
	if (allocated)
	{
		SHARED_INDEX_QUEUE[SHARED_INDEX_TAIL] = gomoku_indice[0] / GOMOKU_TYPE_CONTAINER;
		SHARED_INDEX_TAIL = (SHARED_INDEX_TAIL + 1) % MAX_BOARD;
		allocated = false;
	}
}

BitBoard::~BitBoard()
{
	release();
}

void BitBoard::reset()
{
	int i, j;
	player = BLACK;
	step = 0;
	is_over = false;
	winner = -1;
	zobristKey = 0;

	history[0] = 0;
	for (i = 1; i < 11; i++)
	{
		gomoku_indice[i] = gomoku_indice[0];
		action_indice[i] = action_indice[0];
	}

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{
			bitboards[i][j] = TABLE.BitBoard;
		}
	}
}

BitBoard::BitBoard()
{
	allocated = false;
	allocate();
	reset();
}

BitBoard::BitBoard(std::vector<UC> _history)
{
	allocated = false;
	allocate();
	reset();
	int _step = (int)_history.size();
	for (int i = 0; i < _step; i++)
	{
		move(_history[i]);
	}
}

BitBoard::BitBoard(IVEC _history)
{
	allocated = false;
	allocate();
	reset();
	int _step = (int)_history.size();
	for (int i = 0; i < _step; i++)
	{
		move((UC)_history[i]);
	}
}

void BitBoard::copy(const BitBoard &copyboard)
{
	int i, j;
	player = copyboard.player;
	step = copyboard.step;
	is_over = copyboard.is_over;
	winner = copyboard.winner;
	zobristKey = copyboard.zobristKey;

	memcpy(history, copyboard.history, (copyboard.history[0] + 1) * sizeof(UC));

	for (i = 1; i < 11; i++)
	{
		gomoku_indice[i] = copyboard.gomoku_indice[i] - copyboard.gomoku_indice[0] + gomoku_indice[0];
		action_indice[i] = copyboard.action_indice[i] - copyboard.action_indice[0] + action_indice[0];
	}

	if (gomoku_indice[10] != gomoku_indice[0])
	{
		memcpy(SHARED_GOMOKU_TYPES + gomoku_indice[0], SHARED_ACTIONS + copyboard.gomoku_indice[0],
			   (gomoku_indice[10] - gomoku_indice[0]) * sizeof(UC));
		memcpy(SHARED_DIRECTIONS + gomoku_indice[0], SHARED_DIRECTIONS + copyboard.gomoku_indice[0],
			   (gomoku_indice[10] - gomoku_indice[0]) * sizeof(UC));
	}

	if (action_indice[10] != action_indice[0])
	{
		memcpy(SHARED_ACTIONS + action_indice[0], SHARED_ACTIONS + copyboard.action_indice[0],
			   (action_indice[10] - action_indice[0]) * sizeof(UC));
	}

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{
			bitboards[i][j] = copyboard.bitboards[i][j];
		}
	}
}

BitBoard::BitBoard(const BitBoard &copyboard)
{
	allocated = false;
	allocate();
	copy(copyboard);
}

BitBoard &BitBoard::operator = (const BitBoard &copyboard)
{
	allocate();
	copy(copyboard);
	return *this;
}

GBIT &BitBoard::get_line(UC action, UC direction, int &center)
{
	int row = (int)(action / BOARD_SIZE), col = (int)(action % BOARD_SIZE);
	switch (direction)
	{
	case 0:
		center = col;
		return bitboards[0][row];
	case 1:
		center = row;
		return bitboards[1][col];
	case 2:
		if (row == col)
		{
			center = row;
			return bitboards[2][HALF_SIZE];
		}
		else if (row > col)
		{
			if (row - col > HALF_SIZE)
			{
				center = col;
				return bitboards[2][HALF_SIZE - BOARD_SIZE + row - col];
			}
			else
			{
				center = row;
				return bitboards[2][HALF_SIZE - row + col];
			}
		}
		else
		{
			if (col - row > HALF_SIZE)
			{
				center = row;
				return bitboards[2][HALF_SIZE + BOARD_SIZE - col + row];
			}
			else
			{
				center = col;
				return bitboards[2][HALF_SIZE - row + col];
			}
		}
	case 3:
		if (row == BOARD_SIZE - 1 - col)
		{
			center = row;
			return bitboards[3][HALF_SIZE];
		}
		else if (row > BOARD_SIZE - 1 - col)
		{
			if (row - BOARD_SIZE + 1 + col > HALF_SIZE)
			{
				center = BOARD_SIZE - 1 - col;
				return bitboards[3][HALF_SIZE - BOARD_SIZE + row - BOARD_SIZE + 1 + col];
			}
			else
			{
				center = row;
				return bitboards[3][HALF_SIZE - row + BOARD_SIZE - 1 - col];
			}
		}
		else
		{
			if (BOARD_SIZE - 1 - col - row > HALF_SIZE)
			{
				center = row;
				return bitboards[3][HALF_SIZE + 1 + col + row];
			}
			else
			{
				center = BOARD_SIZE - 1 - col;
				return bitboards[3][HALF_SIZE - row + BOARD_SIZE - 1 - col];
			}
		}
	}
}

extern bool check_five(GBIT &line, int center, int color);

#ifdef BOARD_TEST
extern TestBoard TEST;
#endif

void BitBoard::move(UC action)
{
	bool flag = false;
	int row = (int)(action / BOARD_SIZE), col = (int)(action % BOARD_SIZE);
	int center;
	int color = (int)(player == WHITE);

	for (UC direction = 0; direction < 4; direction++)
	{
		GBIT &line = get_line(action, direction, center);
		line.set(2 * center);
		if (player == BLACK)
		{
			line.set(2 * center + 1, 0);
		}
		if (!flag)
		{
			flag = check_five(line, center, color);
		}
	}

	if (flag)
	{
		winner = player;
		is_over = true;
	}

	step++;
	if (!is_over && step == STONES)
	{
		is_over = true;
		winner = EMPTY;
	}
	history[++history[0]] = action;
	zobristKey ^= TABLE.ZobristTable[2 * action + color];
	player = player_mapping(player);
	check_gomoku_type();
	#ifdef BOARD_TEST
	TEST.test(*this);
	#endif
}

IVEC BitBoard::get_board()
{
	IVEC _board(STONES, EMPTY);
	int color = BLACK;
	for (int i = 1; i <= history[0]; i++)
	{
		_board[history[i]] = color;
		color = player_mapping(color);
	}
	return _board;
}

IVEC BitBoard::get_actions(bool is_player, int gomoku_type)
{
	int index = ((is_player ? player : player_mapping(player)) == BLACK ? 0 : 5) + gomoku_type;
	int begin = action_indice[index - 1], end = action_indice[index];
	IVEC _actions(end - begin, 0);
	for (int i = 0; i < end - begin; i++)
	{
		_actions[i] = (int)SHARED_ACTIONS[begin + i];
	}
	return _actions;
}

void BitBoard::get_fast_actions(bool is_player, int gomoku_type, UC container[], int begin, int &count)
{
	int index = ((is_player ? player : player_mapping(player)) == BLACK ? 0 : 5) + gomoku_type;
	int action_begin = action_indice[index - 1];
	int number = action_indice[index] - action_begin;
	for (int i = 0; i < number; i++)
	{
		container[begin + i] = SHARED_ACTIONS[action_begin + i];
		count++;
	}
}

int BitBoard::count_actions(bool is_player, int gomoku_type)
{
	int index = ((is_player ? player : player_mapping(player)) == BLACK ? 0 : 5) + gomoku_type;
	return action_indice[index] - action_indice[index - 1];
}

bool BitBoard::check_action(bool is_player, int gomoku_type, UC action)
{
	int index = ((is_player ? player : player_mapping(player)) == BLACK ? 0 : 5) + gomoku_type;
	int action_end = action_indice[index];
	for (int i = action_indice[index - 1]; i < action_end; i++)
	{
		if (SHARED_ACTIONS[i] == action)
		{
			return true;
		}
	}
	return false;
}