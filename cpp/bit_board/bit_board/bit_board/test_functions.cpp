#include "bit_board.h"

void print_board(BitBoard &board)
{
	IVEC _board = board.get_board();
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		printf("%2d ", i);
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			if (_board[i*BOARD_SIZE + j] == EMPTY)
			{
				std::cout << "_" << ' ';
			}
			else
			{
				std::cout << _board[i*BOARD_SIZE + j] << ' ';
			}

		}
		std::cout << std::endl;
	}
	printf("   ");
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (i < 10)
		{
			printf("%d ", i);
		}
		else
		{
			printf("1 ");
		}
	}
	printf("\n   ");
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (i < 10)
		{
			printf("  ");
		}
		else
		{
			printf("%d ", i - 10);
		}
	}
	printf("\n");
}

void print_potential_actions(BitBoard &board)
{
	int actions[100] = { 0 };
	for (int is_player = 1; is_player >= 0; is_player--)
	{
		std::cout << "is player: " << is_player << std::endl;
		for (int gt = OPEN_FOUR; gt <= OPEN_TWO; gt++)
		{
			IVEC potential_actions = board.get_actions((bool)is_player, gt);
			if (potential_actions.size() == 0)
			{
				continue;
			}
			std::cout << "gomoku type: " << gt << std::endl << " | ";
			for (IVEC::iterator act = potential_actions.begin(); act != potential_actions.end(); act++)
			{
				std::cout << (*act) / BOARD_SIZE << " " << (*act) % BOARD_SIZE << " | ";
			}
			std::cout << std::endl;
		}
	}
}