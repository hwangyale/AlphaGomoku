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
}

BitBoard::~BitBoard()
{
	SHARED_INDEX_QUEUE[SHARED_INDEX_TAIL] = gomoku_indice[0] / GOMOKU_TYPE_CONTAINER;
	SHARED_INDEX_TAIL = (SHARED_INDEX_TAIL + 1) % MAX_BOARD;
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

void BitBoard::move(UC action)
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
	zobristKey ^= zobrist[2 * action + (player == BLACK ? 0 : 1)];
	player = player == BLACK ? WHITE : BLACK;
	check_gomoku_type();
}

#ifdef DEBUG
int main()
{
	getchar();
	return 0;
}

#endif