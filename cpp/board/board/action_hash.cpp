#include "board.h"

ActionHash::ActionHash()
{
	memset(action_counts, 0, sizeof(action_counts));
	for (int i = 0; i < 10; i++)
	{
		memset(action_stacks[i], 0, sizeof(action_stacks[i]));
		memset(actions[i], 0, sizeof(actions[i]));
	}
}

void ActionHash::reset()
{
	for (int i = 0; i < 10; i++)
	{
		while (action_counts[i] > 0)
		{
			actions[i][action_stacks[i][--action_counts[i]]] = false;
		}
	}
}

bool ActionHash::check(int gt, int action)
{
	bool in_table = actions[gt][action];
	if (!in_table)
	{
		action_stacks[gt][action_counts[gt]++] = action;
		actions[gt][action] = true;
	}
	return in_table;
}