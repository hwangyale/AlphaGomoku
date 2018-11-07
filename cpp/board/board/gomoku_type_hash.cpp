#include "board.h"

GomokuTypeHash::GomokuTypeHash()
{
	memset(values, 0, sizeof(values));
	memset(next, 0, sizeof(next));
	memset(heads, 0, sizeof(heads));
	memset(head_stack, 0, sizeof(head_stack));
	hashed_number = 1;
}

void GomokuTypeHash::reset()
{
	while (hashed_number > 1)
	{
		heads[head_stack[--hashed_number]] = 0;
	}
}

bool GomokuTypeHash::in_table(U64 code)
{
	hash = (int)(code % GOMOKU_TYPE_CONTAINER);
	for (pointer = heads[hash]; pointer != 0; pointer = next[pointer])
	{
		if (values[pointer] == code)
		{
			return true;
		}
	}

	values[hashed_number] = code;
	next[hashed_number] = heads[hash];
	heads[hash] = hashed_number;
	head_stack[hashed_number++] = hash;
	return false;
}