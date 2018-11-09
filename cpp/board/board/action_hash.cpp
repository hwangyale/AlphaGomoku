#include "board.h"

ActionHash::ActionHash()
{
	memset(action_counts[0], 0, sizeof(action_counts[0]));
	memset(action_counts[1], 0, sizeof(action_counts[1]));
	for (int i = 0; i < 5; i++)
	{
		memset(action_stacks[0][i], 0, sizeof(action_stacks[0][i]));
		memset(action_stacks[1][i], 0, sizeof(action_stacks[1][i]));
		memset(actions[0][i], 0, sizeof(actions[0][i]));
		memset(actions[1][i], 0, sizeof(actions[1][i]));
	}
}

void ActionHash::reset()
{
	for (int i = 0; i < 5; i++)
	{
		while (action_counts[0][i] > 0)
		{
			actions[0][i][action_stacks[0][i][--action_counts[0][i]]] = false;
		}
		while (action_counts[1][i] > 0)
		{
			actions[1][i][action_stacks[1][i][--action_counts[1][i]]] = false;
		}
	}
}

bool ActionHash::check(int gt, int action, int color)
{
	bool in_table = actions[color-1][gt-1][action];
	if (!in_table)
	{
		action_stacks[color-1][gt-1][action_counts[color-1][gt-1]++] = action;
		actions[color-1][gt-1][action] = true;
	}
	return in_table;
}