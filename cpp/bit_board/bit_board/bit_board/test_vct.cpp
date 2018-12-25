#ifdef VCT_TEST
#include "bit_board.h"

extern void print_board(BitBoard &board);
extern void print_potential_actions(BitBoard &board);

int main()
{
	/*int _actions[16] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
	5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
	8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5 };
	IVEC actions(_actions, _actions + 16);*/

	int _actions[16] = {
		8 * 15 + 7, 9 * 15 + 7, 6 * 15 + 7, 9 * 15 + 8, 7 * 15 + 8, 6 * 15 + 9,
		7 * 15 + 6, 7 * 15 + 9, 9 * 15 + 9, 8 * 15 + 8, 10 * 15 + 6, 8 * 15 + 6,
		7 * 15 + 4, 6 * 15 + 10, 5 * 15 + 11, 7 * 15 + 7
	};
	IVEC actions(_actions, _actions + 16);

	BitBoard board;
	board.allocate();

	for (int i = 0; i < 16; i++)
	{
		board.move(actions[i]);
		//print_board(board);
		//std::cout << "player: " << (int)board.player << std::endl;
		//print_potential_actions(board);
		//std::cout << std::endl;
		//getchar();
	}

	//board.move(5 * 15 + 7);
	//board.move(5 * 15 + 3);
	//board.move(8 * 15 + 4);
	//board.move(7 * 15 + 5);
	//board.move(5 * 15 + 8);
	//board.move(5 * 15 + 4);
	//board.move(9 * 15 + 5);
	//board.move(10 * 15 + 4);
	//board.move(10 * 15 + 6);
	//board.move(7 * 15 + 3);
	//board.move(12 * 15 + 8);
	//board.move(11 * 15 + 7);
	//board.move(8 * 15 + 8);
	//board.move(9 * 15 + 9);
	//board.move(8 * 15 + 10);
	//board.move(8 * 15 + 9);
	//board.move(7 * 15 + 9);
	//board.move(9 * 15 + 11);

	//board.move(5 * 15 + 7);
	//board.move(6 * 15 + 4);
	//board.move(5 * 15 + 4);
	//board.move(5 * 15 + 3);
	//print_board(board);
	//print_potential_actions(board);

	/*print_board(board);

	clock_t start = clock();
	int vct_action = vct(board, 14, 3600);
	std::cout << vct_action / BOARD_SIZE << " " << vct_action % BOARD_SIZE << std::endl;
	printf("time: %.4f\n", (double)(clock() - start));*/

	int action;
	while (!board.is_over)
	{
		if (board.player == BLACK)
		{
			clock_t start = clock();
			action = vct(board, 12, 3600);
			std::cout << "action: " << action << std::endl;
			printf("vct time: %.4fms\n", (double)(clock() - start));
			printf("move: %d %d\n", action / BOARD_SIZE, action % BOARD_SIZE);
		}
		else
		{
			int row, col;
			scanf("%d %d", &row, &col);
			action = row * BOARD_SIZE + col;
		}
		if (action < 0)
		{
			print_board(board);
			printf("-1\n");
			break;
		}
		board.move(action);
		print_board(board);
	}

	return 0;
}

#endif