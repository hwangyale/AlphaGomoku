#ifdef TEST_FAST_BOARD
#include "board.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

class ActionGenerator
{
public:
	IVEC actions;
	ISET action_set;
	int get_action();
};

int ActionGenerator::get_action()
{
	int action;
	do
	{
		action = (int)(rand()) % STONES;
	} while (action_set.find(action) != action_set.end());
	actions.push_back(action);
	action_set.insert(action);
	if (action > STONES || action < 0)
	{
		std::cout << action << std::endl;
	}
	return action;
}

int main()
{
	FastBoard fast_board;
	Board board;
	ActionGenerator actionGenerator;
	int test_times, t, seed;
	double count = 0;
	clock_t start;
	
	scanf("%d", &test_times);

	seed = (int)clock();

	start = clock();
	srand(seed);
	for (t = 0; t < test_times; t++)
	{
		//std::cout << t << std::endl;
		fast_board = FastBoard();
		actionGenerator = ActionGenerator();
		while (!fast_board.is_over)
		{
			count += 1.0;
			fast_board.move(actionGenerator.get_action(), true);
		}
	}
	printf("average FastBoard move time: %.04f\n", (double)((clock() - start)) / count);

	start = clock();
	srand(seed);
	for (t = 0; t < test_times; t++)
	{
		//std::cout << t << std::endl;
		board = Board();
		actionGenerator = ActionGenerator();
		while (!board.is_over)
		{
			count += 1.0;
			board.move(actionGenerator.get_action());
		}
	}
	printf("average Board move time: %.04f\n", (double)((clock() - start)) / count);
	getchar();
	return 0;
}

#endif