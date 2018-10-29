#include "board.h"
#include <iterator>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "time.h"
#include <iostream>

#ifdef DEBUG
double set_proof_and_disproof_time = 0.0;
double evaluate_time = 0.0;
#endif // DEBUG


#define TABLE_SIZE 10000

#define OR 0
#define AND 1
#define INF 10000000

std::unordered_map<U64, int> BLACK_VCT_TABLE, WHITE_VCT_TABLE;

class Node
{
public:
	int node_type, depth, value, selected_node = -1, proof, disproof;
	Node *parent;
	std::unordered_map<int, Node *> children;
	bool expanded = false;

	Node(int _node_type, Board &board, int _depth, std::unordered_map<U64, bool>
		 &cache_hashing_table, Node *_parent = NULL, int _value = -1);

	void set_proof_and_disproof(std::unordered_map<U64, bool> &cache_hashing_table);
	void set_proof_and_disproof(int &_proof, int &_disproof, std::unordered_map<U64, bool> &cache_hashing_table);

	void develop(int positions[], int values[], std::vector<Board> &boards, std::unordered_map<U64, bool> &cache_hashing_table);

	Node *update(std::unordered_map<U64, bool> &cache_hashing_table);
};


std::unordered_map<Node *, Board> NODE_BOARD_TABLE;

Node::Node(int _node_type, Board &board, int _depth, 
		   std::unordered_map<U64, bool> &cache_hashing_table, 
		   Node *_parent, int _value)
{
	node_type = _node_type;
	NODE_BOARD_TABLE[this] = board;
	depth = _depth;
	parent = _parent;
	value = _value;

	set_proof_and_disproof(cache_hashing_table);
}

void Node::set_proof_and_disproof(std::unordered_map<U64, bool> &cache_hashing_table)
{
	clock_t start = clock();


	std::unordered_map<int, Node *>::iterator child_it;

	std::vector<std::unordered_map<int, Node *>::iterator> erase_positions;

	if (expanded)
	{
		if (node_type)
		{
			proof = 0;
			disproof = INF;

			for (child_it = children.begin(); child_it != children.end(); child_it++)
			{
				if ((child_it->second)->proof == 0)
				{
					erase_positions.push_back(child_it);
				}
				proof += (child_it->second)->proof;
				if (disproof > (child_it->second)->disproof)
				{
					disproof = (child_it->second)->disproof;
					selected_node = child_it->first;
				}
			}
			//std::cout << selected_node << std::endl;
		}
		else
		{
			proof = INF;
			disproof = 0;
			for (child_it = children.begin(); child_it != children.end(); child_it++)
			{
				if ((child_it->second)->disproof == 0)
				{
					erase_positions.push_back(child_it);
				}
				disproof += (child_it->second)->disproof;
				if (proof > (child_it->second)->proof)
				{
					proof = (child_it->second)->proof;
					selected_node = child_it->first;
				}
			}
			//std::cout << selected_node << std::endl;
		}

		for (int idx = 0; idx < (int)erase_positions.size(); idx++)
		{
			delete erase_positions[idx]->second;
			children.erase(erase_positions[idx]);
		}

		if (node_type == OR)
		{
			Board board = NODE_BOARD_TABLE[this];
			if (proof == 0)
			{
				U64 key = board.zobristKey;
				if (board.player == BLACK)
				{
					BLACK_VCT_TABLE[key] = selected_node;
				}
				else
				{
					WHITE_VCT_TABLE[key] = selected_node;
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

#ifdef DEBUG
	set_proof_and_disproof_time += (double)(clock() - start) / CLOCKS_PER_SEC;
#endif // DEBUG

}

void Node::set_proof_and_disproof(int &_proof, int &_disproof, std::unordered_map<U64, bool> &cache_hashing_table)
{
	set_proof_and_disproof(cache_hashing_table);
	_proof = proof;
	_disproof = proof;
}

void Node::develop(int positions[], int values[], std::vector<Board> &boards, std::unordered_map<U64, bool> &cache_hashing_table)
{
	int _node_type = node_type == OR ? AND : OR, _depth = depth + 1;
	for (int idx = 0; idx < (int)boards.size(); idx++)
	{
		children[positions[idx]] = new Node(_node_type, boards[idx], _depth, cache_hashing_table, this, values[idx]);
	}
	expanded = true;
}

Node *Node::update(std::unordered_map<U64, bool> &cache_hashing_table)
{
	int tmp_proof = proof, tmp_disproof = disproof, _proof, _disproof;
	set_proof_and_disproof(_proof, _disproof, cache_hashing_table);
	if (parent != NULL && (tmp_proof != _proof || tmp_disproof != _disproof))
	{
		return parent->update(cache_hashing_table);
	}
	return this;
}

int evaluate(Board &board, int depth, int max_depth, std::vector<int> &positions, 
    		 std::unordered_map<U64, bool> &cache_hashing_table, int player)
{
	U64 zobrisKey = board.zobristKey;
	std::unordered_map<U64, int> &player_vct = player == BLACK ? BLACK_VCT_TABLE : WHITE_VCT_TABLE;
	std::unordered_map<U64, int> &opponent_vct = player != BLACK ? BLACK_VCT_TABLE : WHITE_VCT_TABLE;

	if (player_vct.find(zobrisKey) != player_vct.end())
	{
		positions.push_back(player_vct[zobrisKey]);
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
	
	ISET &player_open_four = board._get_positions(true, OPEN_FOUR);
	if (player_open_four.size() > 0)
	{
		std::copy(player_open_four.begin(), player_open_four.end(), std::back_inserter(positions));
		return board.player == player ? 1 : 0;
	}
	
	ISET &player_four = board._get_positions(true, FOUR);
	if (player_four.size() > 0)
	{
		std::copy(player_four.begin(), player_four.end(), std::back_inserter(positions));
		return board.player == player ? 1 : 0;
	}

	ISET &opponent_open_four = board._get_positions(false, OPEN_FOUR);
	if (opponent_open_four.size() > 0)
	{
		std::copy(opponent_open_four.begin(), opponent_open_four.end(), std::back_inserter(positions));
		return board.player != player ? 1 : 0;
	}
	
	ISET &opponent_four = board._get_positions(false, FOUR);
	if (opponent_four.size() > 1)
	{
		std::copy(opponent_four.begin(), opponent_four.end(), std::back_inserter(positions));
		return board.player != player ? 1 : 0;
	}

	ISET &player_open_three = board._get_positions(true, OPEN_THREE);
	ISET &player_three = board._get_positions(true, THREE);
	ISET &player_open_two = board._get_positions(true, OPEN_TWO);
	ISET &player_two = board._get_positions(true, TWO);
	ISET &opponent_open_three = board._get_positions(false, OPEN_THREE);
	ISET &opponent_three = board._get_positions(false, THREE);
	ISET &opponent_open_two = board._get_positions(false, OPEN_TWO);
	ISET &opponent_two = board._get_positions(false, TWO);

	int value;
	if (opponent_four.size() == 1)
	{
		if (board.player == player)
		{
			if (player_open_three.size() > 0)
			{
				value = unknown;
				std::copy(opponent_four.begin(), opponent_four.end(), std::back_inserter(positions));
			}
			else
			{
				for (ISET::iterator idx = opponent_four.begin(); idx != opponent_four.end(); idx++)
				{
					if (player_open_two.find(*idx) != player_open_two.end() ||
						player_three.find(*idx) != player_three.end())
					{
						positions.push_back(*idx);
					}
				}
				if (positions.size() > 0)
				{
					value = unknown;
				}
				else
				{
					value = 0;
					std::copy(opponent_four.begin(), opponent_four.end(), std::back_inserter(positions));
				}
			}
		}
		else
		{
			value = unknown;
			std::copy(opponent_four.begin(), opponent_four.end(), std::back_inserter(positions));
		}

		return value;
	}

	if (player_open_three.size() > 0)
	{
		std::copy(player_open_three.begin(), player_open_three.end(), std::back_inserter(positions));
		return board.player == player ? 1 : 0;
	}

	if (board.player == player)
	{
		if (opponent_open_three.size() > 0)
		{
			ISET tmp_positions;
			for (ISET::iterator idx = opponent_open_three.begin(); idx != opponent_open_three.end(); idx++)
			{
				if (player_open_two.find(*idx) != player_open_two.end())
				{
					tmp_positions.insert(*idx);
				}
			}
			tmp_positions.insert(player_three.begin(), player_three.end());

			if (tmp_positions.size() > 0)
			{
				value = unknown;
				std::copy(tmp_positions.begin(), tmp_positions.end(), std::back_inserter(positions));
			}
			else
			{
				value = 0;
				std::copy(opponent_open_three.begin(), opponent_open_three.end(), std::back_inserter(positions));
			}
		}
		else
		{
			ISET tmp_positions;
			tmp_positions.insert(player_open_two.begin(), player_open_two.end());
			tmp_positions.insert(player_three.begin(), player_three.end());
			if (tmp_positions.size() > 0)
			{
				value = unknown;
				std::copy(tmp_positions.begin(), tmp_positions.end(), std::back_inserter(positions));
			}
			else
			{
				value = 0;
			}
		}
	}
	else
	{
		if (opponent_open_three.size() > 0)
		{
			value = unknown;
			ISET tmp_positions;
			tmp_positions.insert(opponent_open_three.begin(), opponent_open_three.end());
			tmp_positions.insert(player_three.begin(), player_three.end());
			std::copy(tmp_positions.begin(), tmp_positions.end(), std::back_inserter(positions));
		}
		else
		{
			value = 0;
		}
	}
	return value;
}

void delete_children(Node &root)
{
	if (root.children.size() > 0)
	{
		for (std::unordered_map<int, Node *>::iterator idx = root.children.begin();
			 idx != root.children.end(); idx++)
		{
			delete_children(*(idx->second));
			delete idx->second;
		}
	}
}

int vct(Board &board, int max_depth, double max_time)
{
	if (board.history.size() < 6)
	{
		return -1;
	}

	Board copy_board = board;
	int player = copy_board.player, value;
	std::unordered_map<U64, bool> cache_hashing_table;
	std::vector<int> root_positions;

	value = evaluate(copy_board, 0, max_depth, root_positions, cache_hashing_table, player);
	if (value >= 0)
	{
		if (value)
		{
			return root_positions[0];
		}
		else
		{
			return -1;
		}
	}

	std::unordered_map<U64, std::vector<int>> position_table;
	position_table[copy_board.zobristKey] = root_positions;

	int depth;
	std::vector<int>::iterator int_idx;

	NODE_BOARD_TABLE = std::unordered_map<Node *, Board>();
	NODE_BOARD_TABLE.rehash(TABLE_SIZE);

	Node root(OR, copy_board, 0, cache_hashing_table);
	Node *node = &root;
	clock_t start = clock();
	while (root.proof != 0 && root.disproof != 0 &&
		   (double)(clock() - start) / CLOCKS_PER_SEC < max_time)
	{
		while (node->selected_node != -1) node = node->children[node->selected_node];
		depth = node->depth + 1;
		Board &tmp_board = NODE_BOARD_TABLE[node];
		std::vector<int> &tmp_positions = position_table[tmp_board.zobristKey];
		int poses[STONES], values[STONES], count = 0;
		std::vector<Board> boards;
		for (int_idx = tmp_positions.begin(); int_idx != tmp_positions.end(); int_idx++)
		{
			Board child_board = tmp_board;
			child_board.move(*int_idx, false);
			boards.push_back(child_board);
			poses[count] = *int_idx;
			std::vector<int> positions;

#ifdef DEBUG
			clock_t start = clock();
#endif // DEBUG

			values[count++] = evaluate(child_board, depth, max_depth, positions, cache_hashing_table, player);

#ifdef DEBUG
			evaluate_time += (double)(clock() - start) / CLOCKS_PER_SEC;
#endif // DEBUG


			position_table[child_board.zobristKey] = positions;
		}
		node->develop(poses, values, boards, cache_hashing_table);
		node = node->update(cache_hashing_table);
	}

	value = root.proof == 0 ? root.selected_node : -1;
	delete_children(root);

	//std::cout << NODE_BOARD_TABLE.size() << std::endl;
	//std::cout << BLACK_VCT_TABLE.size() << " " << WHITE_VCT_TABLE.size() << std::endl;
	return value;
}

//int vct(Board &board, int max_depth, double max_time)
//{
//	clock_t func_start = clock();
//	int position = -1;
//	for (int depth = 1; depth <= max_depth; depth++)
//	{
//		if ((double)(clock() - func_start) / CLOCKS_PER_SEC >= max_time)
//		{
//			break;
//		}
//		position = _vct(board, depth, max_time - (double)(clock() - func_start) / CLOCKS_PER_SEC);
//		if (position >= 0)
//		{
//			break;
//		}
//	}
//	return position;
//}