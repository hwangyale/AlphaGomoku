#ifdef GOMOKU_TABLE_TEST
#include "bit_board.h"

void print_line(GBIT &line)
{
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (line[2 * i] == 0)
		{
			if (line[2 * i + 1] == 0)
			{
				printf("*");
			}
			else
			{
				printf("_");
			}
		}
		else
		{
			printf("%d", line[2 * i + 1] + 1);
		}
	}
	printf("\n");
}

GBIT get_sample()
{
	int length = (rand() % BOARD_SIZE) + 1;
	int center = rand() % length + (length <= HALF_SIZE ? 0 : (BOARD_SIZE - length - 1));
	int color;
	GBIT line;
	line.reset();
	for (int i = center - (RANGE - 1) / 2; i <= center + (RANGE - 1) / 2; i++)
	{
		if (i < 0 || i >= BOARD_SIZE) continue;
		if (length <= HALF_SIZE && i >= length) continue;
		if (length > HALF_SIZE && i < BOARD_SIZE - length) continue;
		color = rand() % 3;
		switch (color)
		{
		case EMPTY:
			line.set(2 * i + 1);
			break;
		case BLACK:
			line.set(2 * i);
			break;
		case WHITE:
			line.set(2 * i);
			line.set(2 * i + 1);
			break;
		default:
			break;
		}
	}
	return line;
}

extern GomokuTypeTable GOMOKU_TYPE_TABLE;

int main()
{
	int i, j;
	GBIT line;
	int count;
	int container[BOARD_SIZE];
	for (i = 0; i < 10000; i++)
	{
		line = get_sample();
		count = 0;
		GOMOKU_TYPE_TABLE.get_actions(true, OPEN_TWO, line, container, 0, count);
		if (count > 0)
		{
			std::sort(container, container + count);
			std::cout << line << std::endl;
			print_line(line);
			printf("count: %d    | ", count);
			for (j = 0; j < count; j++)
			{
				printf("%d | ", container[j]);
			}
			printf("\n");
		}
	}
	return 0;
}

#endif

