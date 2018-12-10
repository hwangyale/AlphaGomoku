#ifdef BOARD_TEST
#include "bit_board.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

TestBoard TEST;

TestBoard::TestBoard()
{
	max_pointer = 0;
	max_gomoku_type_number = 0;
	max_action_number = 0;
}

void TestBoard::test(BitBoard &board)
{
	max_pointer = MAX(max_pointer, board.gomoku_indice[0]);
	max_gomoku_type_number = MAX(max_gomoku_type_number, board.gomoku_indice[10] - board.gomoku_indice[0]);
	max_action_number = MAX(max_action_number, board.action_indice[10] - board.action_indice[0]);
}

void TestBoard::print()
{
	std::cout << "max pointer: " << max_pointer << std::endl;
	std::cout << "max gomoku type number: " << max_gomoku_type_number << std::endl;
	std::cout << "max action number: " << max_action_number << std::endl;
}

//void print_board(BitBoard &board)
//{
//	IVEC _board = board.get_board();
//	for (int i = 0; i < BOARD_SIZE; i++)
//	{
//		printf("%2d ", i);
//		for (int j = 0; j < BOARD_SIZE; j++)
//		{
//			if (_board[i*BOARD_SIZE + j] == EMPTY)
//			{
//				std::cout << "_" << ' ';
//			}
//			else
//			{
//				std::cout << _board[i*BOARD_SIZE + j] << ' ';
//			}
//
//		}
//		std::cout << std::endl;
//	}
//	printf("   ");
//	for (int i = 0; i < BOARD_SIZE; i++)
//	{
//		if (i < 10)
//		{
//			printf("%d ", i);
//		}
//		else
//		{
//			printf("1 ");
//		}
//	}
//	printf("\n   ");
//	for (int i = 0; i < BOARD_SIZE; i++)
//	{
//		if (i < 10)
//		{
//			printf("  ");
//		}
//		else
//		{
//			printf("%d ", i - 10);
//		}
//	}
//	printf("\n");
//}
//
//void print_potential_actions(BitBoard &board)
//{
//	int actions[100] = { 0 };
//	for (int is_player = 1; is_player >= 0; is_player--)
//	{
//		std::cout << "is player: " << is_player << std::endl;
//		for (int gt = OPEN_FOUR; gt <= OPEN_TWO; gt++)
//		{
//			IVEC potential_actions = board.get_actions((bool)is_player, gt);
//			if (potential_actions.size() == 0)
//			{
//				continue;
//			}
//			std::cout << "gomoku type: " << gt << std::endl << " | ";
//			for (IVEC::iterator act = potential_actions.begin(); act != potential_actions.end(); act++)
//			{
//				std::cout << (*act) / BOARD_SIZE << " " << (*act) % BOARD_SIZE << " | ";
//			}
//			std::cout << std::endl;
//		}
//	}
//}

class GetAction
{
public:
	GetAction();
	int next();
private:
	int actions[STONES];
	int pointer;
};

int numbers[STONES];

bool compare(int i, int j)
{
	return numbers[i] <= numbers[j];
}

GetAction::GetAction()
{
	for (int i = 0; i < STONES; i++)
	{
		numbers[i] = (int)rand();
		actions[i] = i;
	}
	std::sort(actions, actions + STONES, compare);
	pointer = 0;
}

int GetAction::next()
{
	return actions[pointer++];
}

extern void print_board(BitBoard &board);
extern void print_potential_actions(BitBoard &board);

int main()
{
	/*int _actions[16] = { 7 * 15 + 7, 6 * 15 + 7, 6 * 15 + 8, 7 * 15 + 8, 6 * 15 + 6, 6 * 15 + 5,
		5 * 15 + 6, 4 * 15 + 6, 5 * 15 + 5, 4 * 15 + 4, 7 * 15 + 6, 5 * 15 + 9,
		8 * 15 + 6, 9 * 15 + 6, 8 * 15 + 7, 8 * 15 + 5 };
	IVEC actions(_actions, _actions + 16);

	BitBoard board;

	for (int i = 0; i < 16; i++)
	{
		board.move(actions[i]);
		print_board(board);
		std::cout << "player: " << (int)board.player << std::endl;
		print_potential_actions(board);
		std::cout << std::endl;
		getchar();
	}*/

	int N = 10000, act;
	double count = 0;
	BitBoard *board = new BitBoard(true);
	clock_t start = clock();
	srand(0);
	for (int i = 0; i < N; i++)
	{
		BitBoard *board = new BitBoard(true);
		GetAction actionGenerator;
		while (!board->is_over)
		{
			act = actionGenerator.next();
			//std::cout << "action: " << act << std::endl;
			board->move(act);
			count += 1.0;
		}
	}
	printf("move number: %.0f\n", count);
	printf("average move time: %.4fms\n", (double)((clock() - start) / count));
	TEST.print();

	return 0;
}
#endif