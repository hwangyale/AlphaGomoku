#include "board.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "time.h"

void FastBoardTable::reset()
{
	count_table = std::unordered_map<U64, int>();
	board_table = std::unordered_map<U64, FastBoard>();
	keys.clear();
}

FastBoardTable::FastBoardTable()
{
	keys.reserve(max_size);
	reset();
}

bool FastBoardTable::check_table(U64 key, FastBoard &fastboard)
{
	if (board_table.find(key) != board_table.end())
	{
		fastboard = board_table[key];
		return true;
	}
	else if (count_table.find(key) != count_table.end())
	{
		count_table[key]++;
		return false;
	}
	else
	{
		count_table[key] = 1;
		keys.push_back(key);
		return false;
	}
}

void FastBoardTable::set_table(FastBoard &fastboard)
{
	U64 key = fastboard.zobristKey;
	if ((count_table.find(key) != count_table.end() && count_table[key] > threshold)
		&& board_table.find(key) == board_table.end())
	{
		if (keys.size() >= max_size)
		{
			int count = count_table[key];
			reset();
			count_table[key] = count;
			keys.push_back(key);
		}
		board_table[key] = fastboard;
	}
}