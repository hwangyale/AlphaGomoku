#include "bit_board.h"
#define MAX_BOARD 500000

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
	allocate();
}

BitBoard::BitBoard(std::vector<UC> _history)
{
	allocate();
	int _step = (int)_history.size();
	for (int i = 0; i < _step; i++)
	{
		move(_history[i]);
	}
}

BitBoard::BitBoard(IVEC _history)
{
	allocate();
	int _step = (int)_history.size();
	for (int i = 0; i < _step; i++)
	{
		move((UC)_history[i]);
	}
}

void BitBoard::copy(const BitBoard &copyboard)
{
	allocate();
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
	allocate();
	copy(copyboard);
}

BitBoard &BitBoard::operator = (const BitBoard &copyboard)
{
	copy(copyboard);
	return *this;
}

GBIT &BitBoard::get_line(UC action, UC direction)
{
	int row = (int)(action / BOARD_SIZE), col = (int)(action % BOARD_SIZE);
	switch (direction)
	{
	case 0:
		return bitboards[0][row];
	case 1:
		return bitboards[1][col];
	case 2:
		if (row == col)
		{
			return bitboards[2][HALF_SIZE];
		}
		else if (row > col)
		{
			if (row - col > HALF_SIZE)
			{
				return bitboards[2][HALF_SIZE - BOARD_SIZE + row - col];
			}
			else
			{
				return bitboards[2][HALF_SIZE - row + col];
			}
		}
		else
		{
			if (col - row > HALF_SIZE)
			{
				return bitboards[2][HALF_SIZE - BOARD_SIZE + col - row];
			}
			else
			{
				return bitboards[2][HALF_SIZE - col + row];
			}
		}
	case 3:
		if (row == BOARD_SIZE - 1 - col)
		{
			return bitboards[3][HALF_SIZE];
		}
		else if (row > BOARD_SIZE - 1 - col)
		{
			if (row - BOARD_SIZE + 1 + col > HALF_SIZE)
			{
				return bitboards[3][HALF_SIZE - BOARD_SIZE + row - BOARD_SIZE + 1 + col];
			}
			else
			{
				return bitboards[3][HALF_SIZE - row + BOARD_SIZE - 1 - col];
			}
		}
		else
		{
			if (BOARD_SIZE - 1 - col - row > HALF_SIZE)
			{
				return bitboards[3][HALF_SIZE - 1 - col - row];
			}
			else
			{
				return bitboards[3][HALF_SIZE - BOARD_SIZE + 1 + col + row];
			}
		}
	}
	
}

extern bool check_five(GBIT &line, int center, int color);

void BitBoard::move(UC action)
{
	bool flag;
	int row = (int)(action / BOARD_SIZE), col = (int)(action % BOARD_SIZE);
	int i, j;
	int color = (int)(player == WHITE);

	bitboards[0][row].set(2 * col);
	if (player == WHITE)
	{
		bitboards[0][row].set(2 * col + 1);
	}
	flag = check_five(bitboards[0][row], col, color);

	bitboards[1][col].set(2 * row);
	if (player == WHITE)
	{
		bitboards[1][col].set(2 * row + 1);
	}
	if (!flag)
	{
		flag = check_five(bitboards[1][col], row, color);
	}

	if (row == col)
	{
		i = HALF_SIZE;
		j = row;
	}
	else if (row > col)
	{
		if (col + HALF_SIZE < row)
		{
			i = HALF_SIZE - BOARD_SIZE + row - col;
			j = col;
		}
		else
		{
			i = HALF_SIZE - row + col;
			j = row;
		}
	}
	else
	{
		if (row + HALF_SIZE < col)
		{
			i = HALF_SIZE - BOARD_SIZE + col - row;
			j = row;
		}
		else
		{
			i = HALF_SIZE - col + row;
			j = col;
		}
	}
	bitboards[2][i].set(2 * j);
	if (player == WHITE)
	{
		bitboards[2][i].set(2 * j + 1);
	}
	if (!flag)
	{
		flag = check_five(bitboards[2][i], j, color);
	}

	if (row == BOARD_SIZE - 1 - col)
	{
		i = HALF_SIZE;
		j = row;
	}
	else if (row > BOARD_SIZE - 1 - col)
	{
		if (BOARD_SIZE - 1 - col + HALF_SIZE < row)
		{
			i = HALF_SIZE - BOARD_SIZE + row - BOARD_SIZE + 1 + col;
			j = BOARD_SIZE - 1 - col;
		}
		else
		{
			i = HALF_SIZE - row + BOARD_SIZE - 1 - col;
			j = row;
		}
	}
	else
	{
		if (row + HALF_SIZE < BOARD_SIZE - 1 - col)
		{
			i = HALF_SIZE - BOARD_SIZE + BOARD_SIZE - 1 - col - row;
			j = row;
		}
		else
		{
			i = HALF_SIZE - BOARD_SIZE + 1 + col + row;
			j = BOARD_SIZE - 1 - col;
		}
	}
	bitboards[3][i].set(2 * j);
	if (player == WHITE)
	{
		bitboards[3][i].set(2 * j + 1);
	}
	if (!flag)
	{
		flag = check_five(bitboards[3][i], j, color);
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
}

#ifdef DEBUG
int main()
{
	getchar();
	return 0;
}

#endif