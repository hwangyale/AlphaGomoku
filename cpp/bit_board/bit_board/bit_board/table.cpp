#include "bit_board.h"
#include <stdlib.h>

void Table::init_BitBoard()
{
	for (int i = 0; i < 2 * BOARD_SIZE; i += 2)
	{
		BitBoard.set(i);
	}
}

void Table::init_ZobristTable()
{
	U64 rand_number;
	srand(0);
	for (int i = 0; i < 2 * STONES + 4 + 10; i++)
	{
		rand_number = rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
		ZobristTable[i] = rand_number;
	}
}

void Table::init_Masks()
{
	int i, j, k;
	for (i = 0; i < BOARD_SIZE; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{
			for (k = j - (RANGE - 1) / 2; k <= j + (RANGE - 1) / 2; k++)
			{
				if (k < 0 || k >= BOARD_SIZE) continue;
				Masks[0][i][j].set(2 * k);
				Masks[0][i][j].set(2 * k + 1);
			}
		}
	}

	for (i = 0; i < BOARD_SIZE; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{
			for (k = j - (RANGE - 1) / 2; k <= j + (RANGE - 1) / 2; k++)
			{
				if (k < 0 || k >= BOARD_SIZE) continue;
				Masks[1][i][j].set(2 * k);
				Masks[1][i][j].set(2 * k + 1);
			}
		}
	}

	for (i = 0; i < BOARD_SIZE; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{
			for (k = j - (RANGE - 1) / 2; k <= j + (RANGE - 1) / 2; k++)
			{
				if (i < HALF_SIZE && (k < 0 || k > i)) continue;
				if (i >= HALF_SIZE && (k < i - HALF_SIZE || k >= BOARD_SIZE)) continue;
				Masks[2][i][j].set(2 * k);
				Masks[2][i][j].set(2 * k + 1);
			}
		}
	}

	for (i = 0; i < BOARD_SIZE; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{
			for (k = j - (RANGE - 1) / 2; k <= j + (RANGE - 1) / 2; k++)
			{
				if (i < HALF_SIZE && (k < 0 || k > i)) continue;
				if (i >= HALF_SIZE && (k < i - HALF_SIZE || k >= BOARD_SIZE)) continue;
				Masks[3][i][j].set(2 * k);
				Masks[3][i][j].set(2 * k + 1);
			}
		}
	}
}

GBIT &Table::get_mask(UC action, UC direction)
{
	int row = action / BOARD_SIZE, col = action % BOARD_SIZE;
	if (direction < 2)
	{
		return Masks[direction][row][col];
	}
	else if (direction == 2)
	{
		if (row == col)
		{
			return Masks[2][HALF_SIZE][row];
		}

		int i, j;
		if (row < col)
		{
			i = (row + HALF_SIZE < col) ? BOARD_SIZE + row - col - 1 : HALF_SIZE + col - row;
			j = i < HALF_SIZE ? row : (i - HALF_SIZE) + row;
			return Masks[2][i][j];
		}
		else
		{
			i = (col + HALF_SIZE < row) ? BOARD_SIZE + col - row - 1 : HALF_SIZE + row - col;
			j = i < HALF_SIZE ? col : (i - HALF_SIZE) + col;
			return Masks[2][i][j];
		}
		
	}
	else
	{

	}
}

Table::Table()
{
	init_BitBoard();
	init_ZobristTable();
	init_Masks();
}

Table TABLE = Table();