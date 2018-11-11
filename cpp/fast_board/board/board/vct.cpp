#include "board.h"
#include <iterator>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include "time.h"
#include <iostream>
#define NODE_TABLE_CONTAINER 700000

#ifdef FAST_DEBUG
double set_proof_and_disproof_time = 0.0;
double evaluate_time = 0.0;
#endif // FAST_DEBUG


#define TABLE_SIZE 10000

#define OR 0
#define AND 1
#define INF 10000000

std::unordered_map<U64, int> FAST_BLACK_VCT_TABLE, FAST_WHITE_VCT_TABLE;

class FastNode
{
public:
	int pointer; 
	int parent, action, first_child = -1, previous_sibling, next_sibling;
	int node_type, depth, value, selected_node = -1, proof, disproof;
	bool expanded = false;

	FastNode();

	FastNode(int _pointer, int _node_type, int _depth, std::unordered_map<U64, bool> &cache_hashing_table, 
			 int _parent = -1, int _action = -1, int _previous_sibling = -1, 
			 int _next_sibling = -1, int _value = -1);

	FastNode(FastNode &copynode);

	void set_proof_and_disproof(std::unordered_map<U64, bool> &cache_hashing_table);
	void set_proof_and_disproof(int &_proof, int &_disproof, std::unordered_map<U64, bool> &cache_hashing_table);

	void develop(int actions[], int values[], int begin, int end, std::unordered_map<U64, bool> &cache_hashing_table);

	int update(std::unordered_map<U64, bool> &cache_hashing_table);
};

FastNode FAST_NODE_TABLE[NODE_TABLE_CONTAINER];
FastBoard FAST_NODE_BOARD_TABLE[NODE_TABLE_CONTAINER];
int FAST_NODE_NUMBER;

FastNode::FastNode()
{
	pointer = -1;
	parent = -1;
	action = -1;
	first_child = -1;
	previous_sibling = -1;
	next_sibling = -1;
	node_type = -1;
	depth = -1;
	value = -1;
	selected_node = -1;
	proof = -1;
	disproof = -1;
	expanded = false;
}

FastNode::FastNode(int _pointer, int _node_type, int _depth, std::unordered_map<U64, bool> &cache_hashing_table,
				   int _parent, int _action, int _previous_sibling, 
			       int _next_sibling, int _value)
{
	pointer = _pointer;
	node_type = _node_type;
	depth = _depth;
	parent = _parent;
	action = _action;
	previous_sibling = _previous_sibling;
	next_sibling = _next_sibling;
	value = _value;

	set_proof_and_disproof(cache_hashing_table);
}

FastNode::FastNode(FastNode &copynode)
{
	pointer = copynode.pointer;
	parent = copynode.parent;
	action = copynode.action;
	first_child = copynode.first_child;
	previous_sibling = copynode.previous_sibling;
	next_sibling = copynode.next_sibling;
	node_type = copynode.node_type;
	depth = copynode.depth;
	value = copynode.value;
	selected_node = copynode.selected_node;
	proof = copynode.proof;
	disproof = copynode.disproof;
	expanded = copynode.expanded;
}

void FastNode::set_proof_and_disproof(std::unordered_map<U64, bool> &cache_hashing_table)
{
	#ifdef FAST_DEBUG
	clock_t start = clock();
	#endif // FAST_DEBUG

	int child_pointer;

	if (expanded)
	{
		if (node_type)
		{
			proof = 0;
			disproof = INF;
			for (child_pointer = first_child; child_pointer >= 0;
				 child_pointer = FAST_NODE_TABLE[child_pointer].next_sibling)
			{
				FastNode &child_node = FAST_NODE_TABLE[child_pointer];
				if (child_node.proof == 0)
				{
					if (child_node.previous_sibling >= 0)
					{
						FAST_NODE_TABLE[child_node.previous_sibling].next_sibling = child_node.next_sibling;
					}
					if (child_node.next_sibling >= 0)
					{
						FAST_NODE_TABLE[child_node.next_sibling].previous_sibling = child_node.previous_sibling;
					}
				}
				else
				{
					proof += child_node.proof;
				}
				if (disproof > child_node.disproof)
				{
					disproof = child_node.disproof;
					selected_node = child_node.pointer;
				}
			}
		}
		else
		{
			proof = INF;
			disproof = 0;
			for (child_pointer = first_child; child_pointer >= 0;
				 child_pointer = FAST_NODE_TABLE[child_pointer].next_sibling)
			{
				FastNode &child_node = FAST_NODE_TABLE[child_pointer];
				if (child_node.disproof == 0)
				{
					if (child_node.previous_sibling >= 0)
					{
						FAST_NODE_TABLE[child_node.previous_sibling].next_sibling = child_node.next_sibling;
					}
					if (child_node.next_sibling >= 0)
					{
						FAST_NODE_TABLE[child_node.next_sibling].previous_sibling = child_node.previous_sibling;
					}
				}
				else
				{
					disproof += child_node.disproof;
				}
				if (proof > child_node.proof)
				{
					proof = child_node.proof;
					selected_node = child_node.pointer;
				}
			}
		}

		if (node_type == OR)
		{
			FastBoard &board = FAST_NODE_BOARD_TABLE[pointer];
			if (proof == 0)
			{
				U64 key = board.zobristKey;
				if (board.player == BLACK)
				{
					FAST_BLACK_VCT_TABLE[key] = FAST_NODE_TABLE[selected_node].action;
				}
				else
				{
					FAST_WHITE_VCT_TABLE[key] = FAST_NODE_TABLE[selected_node].action;
				}

			}
			else if (disproof == 0)
			{
				cache_hashing_table[board.zobristKey] = true;
			}
		}
	}
	else
	{
		if (value == -1)
		{
			proof = 1;
			disproof = 1 + depth / 2;
		}
		else if (value)
		{
			proof = 0;
			disproof = INF;
		}
		else
		{
			proof = INF;
			disproof = 0;
		}
	}

	#ifdef FAST_DEBUG
	set_proof_and_disproof_time += (double)(clock() - start) / CLOCKS_PER_SEC;
	#endif // FAST_DEBUG

}

void FastNode::set_proof_and_disproof(int &_proof, int &_disproof, std::unordered_map<U64, bool> &cache_hashing_table)
{
	set_proof_and_disproof(cache_hashing_table);
	_proof = proof;
	_disproof = proof;
}

void FastNode::develop(int actions[], int values[], int begin, int end, std::unordered_map<U64, bool> &cache_hashing_table)
{
	int _node_type = node_type == OR ? AND : OR, _depth = depth + 1;
	int _previous_sibling = -1;
	for (int idx = begin; idx < end; idx++)
	{
		FAST_NODE_NUMBER++;
		FAST_NODE_TABLE[FAST_NODE_NUMBER] = FastNode(FAST_NODE_NUMBER, _node_type, _depth, cache_hashing_table,
												     pointer, actions[idx], _previous_sibling, -1, values[idx]);
		_previous_sibling = FAST_NODE_NUMBER;
		if (idx > begin)
		{
			FAST_NODE_TABLE[FAST_NODE_NUMBER - 1].next_sibling = FAST_NODE_NUMBER;
		}
		else
		{
			first_child = FAST_NODE_NUMBER;
		}
	}
	expanded = true;
}

int FastNode::update(std::unordered_map<U64, bool> &cache_hashing_table)
{
	int tmp_proof = proof, tmp_disproof = disproof, _proof, _disproof;
	set_proof_and_disproof(_proof, _disproof, cache_hashing_table);
	if (parent >= 0 && (tmp_proof != _proof || tmp_disproof != _disproof))
	{
		return FAST_NODE_TABLE[parent].update(cache_hashing_table);
	}
	return pointer;
}

int fast_evaluate(FastBoard &board, int depth, int max_depth, int actions[], int begin_index, int &count,
				   std::unordered_map<U64, bool> &cache_hashing_table, int player)
{
	U64 zobrisKey = board.zobristKey;
	std::unordered_map<U64, int> &player_vct = player == BLACK ? FAST_BLACK_VCT_TABLE : FAST_WHITE_VCT_TABLE;
	std::unordered_map<U64, int> &opponent_vct = player != BLACK ? FAST_BLACK_VCT_TABLE : FAST_WHITE_VCT_TABLE;
	static int tmp_actions[100];

	count = 0;

	if (player_vct.find(zobrisKey) != player_vct.end())
	{
		count++;
		actions[begin_index] = player_vct[zobrisKey];
		return 1;
	}
	else if (opponent_vct.find(zobrisKey) != opponent_vct.end())
	{
		return 0;
	}
	else if (cache_hashing_table.find(zobrisKey) != cache_hashing_table.end() && cache_hashing_table[zobrisKey])
	{
		return 0;
	}

	int unknown = depth < max_depth ? -1 : 0;

	if (board.count_actions(true, OPEN_FOUR) > 0)
	{
		board.get_fast_actions(true, OPEN_FOUR, actions, count, begin_index);
		return board.player == player ? 1 : 0;
	}

	if (board.count_actions(true, FOUR) > 0)
	{
		board.get_fast_actions(true, FOUR, actions, count, begin_index);
		return board.player == player ? 1 : 0;
	}

	if (board.count_actions(false, OPEN_FOUR) > 0)
	{
		board.get_fast_actions(false, OPEN_FOUR, actions, count, begin_index);
		return board.player != player ? 1 : 0;
	}

	if (board.count_actions(false, FOUR) > 1)
	{
		board.get_fast_actions(false, FOUR, actions, count, begin_index);
		return board.player != player ? 1 : 0;
	}

	int value;
	if (board.count_actions(false, FOUR) == 1)
	{
		board.get_fast_actions(false, FOUR, actions, count, begin_index);
		if (board.player == player)
		{
			if (board.count_actions(true, OPEN_THREE) > 0)
			{
				value = unknown;
			}
			else
			{
				if (board.check_action(true, THREE, actions[1]))
				{
					value = unknown;
				}
				else if (board.check_action(true, OPEN_TWO, actions[1]))
				{
					value = unknown;
				}
				else
				{
					value = 0;
				}
			}
		}
		else
		{
			value = unknown;
		}
		return value;
	}

	if (board.count_actions(true, OPEN_THREE) > 0)
	{
		board.get_fast_actions(true, OPEN_THREE, actions, count, begin_index);
		return board.player == player ? 1 : 0;
	}

	if (board.player == player)
	{
		if (board.count_actions(false, OPEN_THREE) > 0)
		{
			tmp_actions[0] = 0;
			board.get_fast_actions(false, OPEN_THREE, tmp_actions, tmp_actions[0], 1);
			for (int idx = 1; idx <= tmp_actions[0]; idx++)
			{
				if (board.check_action(true, OPEN_TWO, tmp_actions[idx]))
				{
					actions[begin_index + count++] = tmp_actions[idx];
				}
			}
		}
		else
		{
			board.get_fast_actions(true, OPEN_TWO, actions, count, begin_index);
		}

		int tmp_count = count;
		bool check_repeat = tmp_count > 0;

		board.get_fast_actions(true, THREE, actions, count, begin_index + tmp_count);

		check_repeat = check_repeat & (count > tmp_count);

		if (check_repeat)
		{
			std::bitset<STONES> hash;
			tmp_count = count;
			count = 0;
			for (int idx = 0; idx < tmp_count; idx++)
			{
				if (hash[actions[begin_index + idx]] == 1)
				{
					continue;
				}
				hash.set(actions[begin_index + idx] + 1);
				actions[begin_index + count++] = actions[begin_index + idx];
			}
		}

		if (count > 0)
		{
			value = unknown;
		}
		else
		{
			value = 0;
		}
	}
	else
	{
		if (board.count_actions(false, OPEN_THREE) > 0)
		{
			board.get_fast_actions(true, THREE, actions, count, begin_index);
			board.get_fast_actions(false, OPEN_THREE, actions, count, begin_index + count);

			std::bitset<STONES> hash;
			int tmp_count = count;
			count = 0;
			for (int idx = 0; idx < tmp_count; idx++)
			{
				if (hash[actions[begin_index + idx]] == 1)
				{
					continue;
				}
				hash.set(actions[begin_index + idx] + 1);
				actions[begin_index + count++] = actions[begin_index + idx];
			}

			value = unknown;
		}
		else
		{
			value = 0;
		}
	}
	return value;
}

int shared_actions[2000000], shared_values[2000000], shared_index;

extern int GOMOKU_TYPE_TABLE_INDEX;

int fastVct(FastBoard &board, int max_depth, double max_time)
{
	if (board.history[0] < 6)
	{
		return -1;
	}

	FAST_NODE_NUMBER = 0;

	FastBoard copy_board = board;
	int player = copy_board.player, value;
	std::unordered_map<U64, bool> cache_hashing_table;

	shared_index = 0;
	shared_actions[shared_index] = 0;
	shared_values[shared_index] = 0;

	value = fast_evaluate(copy_board, 0, max_depth, shared_actions, shared_index + 1, shared_actions[shared_index], cache_hashing_table, player);
	if (value >= 0)
	{
		if (value)
		{
			return shared_actions[shared_index + 1];
		}
		else
		{
			return -1;
		}
	}

	std::unordered_map<U64, int> action_table;
	action_table[copy_board.zobristKey] = shared_index;
	shared_index += shared_actions[shared_index] + 1;

	int depth, idx, node_pointer, tmp_action_index, tmp_action;

	FAST_NODE_TABLE[FAST_NODE_NUMBER] = FastNode(FAST_NODE_NUMBER, OR, 0, cache_hashing_table, -1, -1, -1, -1, -1);
	FAST_NODE_BOARD_TABLE[FAST_NODE_NUMBER] = copy_board;
	node_pointer = FAST_NODE_NUMBER;

	FastNode &root = FAST_NODE_TABLE[FAST_NODE_NUMBER];

	clock_t start = clock();
	while (root.proof != 0 && root.disproof != 0 &&
		   (double)(clock() - start) / CLOCKS_PER_SEC < max_time)
	{
		while (FAST_NODE_TABLE[node_pointer].selected_node != -1)
		{
			node_pointer = FAST_NODE_TABLE[node_pointer].selected_node;
		}

		FastNode &node = FAST_NODE_TABLE[node_pointer];

		depth = node.depth + 1;
		FastBoard &tmp_board = FAST_NODE_BOARD_TABLE[node_pointer];
		tmp_action_index = action_table[tmp_board.zobristKey];
		shared_values[tmp_action_index] = 0;

		for (idx = 1; idx <= shared_actions[tmp_action_index]; idx++)
		{
			tmp_action = shared_actions[tmp_action_index + idx];
			FAST_NODE_BOARD_TABLE[FAST_NODE_NUMBER + idx] = tmp_board;

			FastBoard &child_board = FAST_NODE_BOARD_TABLE[FAST_NODE_NUMBER + idx];
			child_board.move(tmp_action, false);

			shared_actions[shared_index] = 0;

			#ifdef FAST_DEBUG
			clock_t tmp_start = clock();
			#endif // FAST_DEBUG
			shared_values[++shared_values[tmp_action_index] + tmp_action_index] = fast_evaluate(child_board,
																				  depth, max_depth, shared_actions, shared_index + 1, 
																				  shared_actions[shared_index], 
																		          cache_hashing_table, player);

			#ifdef FAST_DEBUG
			evaluate_time += (double)(clock() - tmp_start) / CLOCKS_PER_SEC;
			#endif // FAST_DEBUG

			action_table[child_board.zobristKey] = shared_index;
			shared_index += shared_actions[shared_index] + 1;
		}

		node.develop(shared_actions, shared_values, tmp_action_index + 1, tmp_action_index + 1 + shared_actions[tmp_action_index],
				     cache_hashing_table);
		node_pointer = node.update(cache_hashing_table);
	}

	value = root.proof == 0 ? FAST_NODE_TABLE[root.selected_node].action : -1;

	return value;
}
