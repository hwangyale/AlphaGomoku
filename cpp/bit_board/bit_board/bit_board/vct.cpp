#include "bit_board.h"
#include <unordered_map>
#include "time.h"
#define INIT_NODE_TABLE_CONTAINER 2000000

#define INIT_BOARD_TABLE_CONTAINER 1000000
#define SHARED_CONTAINER 5000000
#define VCT_TABLE_LIMIT 2000000
#define OR 0
#define AND 1
#define INF 10000000

extern void print_board(BitBoard &board);
extern void print_potential_actions(BitBoard &board);

std::unordered_map<U64, int> BLACK_VCT_TABLE, WHITE_VCT_TABLE;

UC VCT_SHARED_ACTIONS[SHARED_CONTAINER];
int VCT_SHARED_TABLE[SHARED_CONTAINER], VCT_SHARED_INDEX, VCT_ACTION_INDEX;

class Node
{
public:
	int pointer, board_pointer;
	int parent, action, first_child = -1, next_sibling;
	int node_type, depth, value, selected_node = -1, proof, disproof;
	int shared_index;
	int player;
	U64 zobristKey;
	bool expanded = false;

	Node();
	void reset();
	void set(int _pointer, int _board_pointer, int _node_type, int _depth, std::unordered_map<U64, bool> &cache_table,
			 int _parent = -1, int _action = -1, int _next_sibling = -1, int _value = -1, int _shared_index = -1);

	void set_proof_and_disproof(std::unordered_map<U64, bool> &cache_table);
	void set_proof_and_disproof(int &_proof, int &_disproof, std::unordered_map<U64, bool> &cache_table);

	void develop(int current, int unknown, std::unordered_map<U64, bool> &cache_hashing_table);

	int update(std::unordered_map<U64, bool> &cache_table);
};

std::vector<Node> NODE_TABLE = std::vector<Node>(INIT_NODE_TABLE_CONTAINER);
std::vector<BitBoard> BOARD_TABLE = std::vector<BitBoard>(INIT_BOARD_TABLE_CONTAINER);
IVEC NODE_QUEUE = IVEC(INIT_NODE_TABLE_CONTAINER, 0);
int NODE_POINTER, NODE_QUEUE_HEAD, NODE_QUEUE_TAIL;
IVEC BOARD_QUEUE = IVEC(INIT_BOARD_TABLE_CONTAINER, 0);
int BOARD_POINTER, BOARD_QUEUE_HEAD, BOARD_QUEUE_TAIL;

void initial_tables()
{
	NODE_POINTER = 0;
	NODE_QUEUE_HEAD = 0;
	NODE_QUEUE_TAIL = 0;
	BOARD_POINTER = 0;
	BOARD_QUEUE_HEAD = 0;
	BOARD_QUEUE_TAIL = 0;
}

void release_node(int pointer)
{
	int size = (int)NODE_QUEUE.size();
	NODE_QUEUE[NODE_QUEUE_TAIL] = pointer;
	NODE_QUEUE_TAIL = (NODE_QUEUE_TAIL + 1) % size;
}

int allocate_node()
{
	int pointer;
	int size = (int)NODE_QUEUE.size();
	if (NODE_QUEUE_HEAD != NODE_QUEUE_TAIL)
	{
		pointer = NODE_QUEUE[NODE_QUEUE_HEAD];
		NODE_QUEUE_HEAD = (NODE_QUEUE_HEAD + 1) % size;
	}
	else
	{
		pointer = NODE_POINTER++;
		/*if (NODE_POINTER == size)
		{
			NODE_TABLE.resize(2 * size);
			NODE_QUEUE.resize(2 * size);
			if (NODE_QUEUE_TAIL < NODE_QUEUE_HEAD)
			{
				for (int i = 0; i < NODE_QUEUE_TAIL; i++)
				{
					NODE_QUEUE[size + i] = NODE_QUEUE[i];
				}
				NODE_QUEUE_TAIL += size;
			}
		}*/
	}
	NODE_TABLE[pointer].reset();
	return pointer;
}

void release_board(int pointer)
{
	int size = (int)BOARD_QUEUE.size();
	BOARD_QUEUE[BOARD_QUEUE_TAIL] = pointer;
	BOARD_QUEUE_TAIL = (BOARD_QUEUE_TAIL + 1) % size;
	BOARD_TABLE[pointer].release();
}

int allocate_board()
{
	int pointer;
	int size = (int)BOARD_QUEUE.size();
	if (BOARD_QUEUE_HEAD != BOARD_QUEUE_TAIL)
	{
		pointer = BOARD_QUEUE[BOARD_QUEUE_HEAD];
		BOARD_QUEUE_HEAD = (BOARD_QUEUE_HEAD + 1) % size;
	}
	else
	{
		pointer = BOARD_POINTER++;
		/*if (BOARD_POINTER == size)
		{
			BOARD_TABLE.resize(2 * size);
			BOARD_QUEUE.resize(2 * size);
			if (BOARD_QUEUE_TAIL < BOARD_QUEUE_HEAD)
			{
				for (int i = 0; i < BOARD_QUEUE_TAIL; i++)
				{
					BOARD_QUEUE[size + i] = BOARD_QUEUE[i];
				}
				BOARD_QUEUE_TAIL += size;
			}
		}*/
	}
	BOARD_TABLE[pointer].allocate();
	return pointer;
}

void Node::reset()
{
	pointer = -1;
	board_pointer = -1;
	parent = -1;
	action = -1;
	first_child = -1;
	next_sibling = -1;
	node_type = -1;
	depth = -1;
	value = -1;
	selected_node = -1;
	proof = -1;
	disproof = -1;
	shared_index = -1;
	zobristKey = 0;
	player = -1;
	expanded = false;
}

Node::Node()
{
	reset();
}

void Node::set(int _pointer, int _board_pointer, int _node_type, int _depth, std::unordered_map<U64, bool> &cache_table,
			   int _parent, int _action, int _next_sibling, int _value, int _shared_index)
{
	pointer = _pointer;
	board_pointer = _board_pointer;
	node_type = _node_type;
	depth = _depth;
	parent = _parent;
	action = _action;
	next_sibling = _next_sibling;
	value = _value;
	shared_index = _shared_index;
	zobristKey = BOARD_TABLE[board_pointer].zobristKey;
	player = BOARD_TABLE[board_pointer].player;

	set_proof_and_disproof(cache_table);

	if (value != -1)
	{
		release_board(board_pointer);
	}
}

void Node::set_proof_and_disproof(std::unordered_map<U64, bool> &cache_table)
{
	int child_pointer;
	int previous_pointer = -1;
	int selected_action;

	if (expanded)
	{
		if (node_type)
		{
			proof = 0;
			disproof = INF;
			for (child_pointer = first_child; child_pointer >= 0;
				 child_pointer = NODE_TABLE[child_pointer].next_sibling)
			{
				Node &child_node = NODE_TABLE[child_pointer];
				if (disproof > child_node.disproof)
				{
					disproof = child_node.disproof;
					selected_node = child_node.pointer;
					selected_action = child_node.action;
				}
				if (child_node.proof == 0)
				{
					if (previous_pointer < 0)
					{
						first_child = child_node.next_sibling;
					}
					else
					{
						NODE_TABLE[previous_pointer].next_sibling = child_node.next_sibling;
					}
					release_node(child_pointer);
				}
				else
				{
					proof += child_node.proof;
					previous_pointer = child_pointer;
				}
			}
		}
		else
		{
			proof = INF;
			disproof = 0;
			for (child_pointer = first_child; child_pointer >= 0;
				 child_pointer = NODE_TABLE[child_pointer].next_sibling)
			{
				Node &child_node = NODE_TABLE[child_pointer];
				if (proof > child_node.proof)
				{
					proof = child_node.proof;
					selected_node = child_node.pointer;
					selected_action = child_node.action;
				}
				if (child_node.disproof == 0)
				{
					if (previous_pointer < 0)
					{
						first_child = child_node.next_sibling;
					}
					else
					{
						NODE_TABLE[previous_pointer].next_sibling = child_node.next_sibling;
					}
					release_node(child_pointer);
				}
				else
				{
					disproof += child_node.disproof;
					previous_pointer = child_pointer;
				}
			}
		}

		if (node_type == OR)
		{
			if (proof == 0)
			{
				if (player == BLACK)
				{
					BLACK_VCT_TABLE[zobristKey] = selected_action;
				}
				else
				{
					WHITE_VCT_TABLE[zobristKey] = selected_action;
				}

			}
			else if (disproof == 0)
			{
				cache_table[zobristKey] = true;
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
}

void Node::set_proof_and_disproof(int &_proof, int &_disproof, std::unordered_map<U64, bool> &cache_table)
{
	set_proof_and_disproof(cache_table);
	_proof = proof;
	_disproof = proof;
}

void Node::develop(int current, int unknown, std::unordered_map<U64, bool> &cache_table)
{
	int begin = VCT_SHARED_TABLE[2 * shared_index];
	int end = begin + VCT_SHARED_TABLE[2 * shared_index + 1];
	int _node_type = node_type == OR ? AND : OR, _depth = depth + 1;
	int _pointer = -1, _next_pointer = -1, _board_pointer;
	int _action, _value;
	BitBoard &board = BOARD_TABLE[board_pointer];
	if (!board.allocated) printf("not allocated\n");
	if (expanded) printf("expanded\n");
	BitBoard backup = board;
	for (int idx = end - 1; idx >= begin; idx--)
	{
		_action = (int)VCT_SHARED_ACTIONS[idx];
		_pointer = allocate_node();
		_board_pointer = allocate_board();
		BitBoard &_board = BOARD_TABLE[_board_pointer];
		_board = board;
		_board.move(_action);

		VCT_SHARED_TABLE[2 * VCT_SHARED_INDEX] = VCT_ACTION_INDEX;
		VCT_SHARED_TABLE[2 * VCT_SHARED_INDEX + 1] = 0;
		_value = _board.evaluate(VCT_SHARED_ACTIONS, VCT_ACTION_INDEX, VCT_SHARED_TABLE[2 * VCT_SHARED_INDEX + 1],
								 current, cache_table, unknown);

		NODE_TABLE[_pointer].set(_pointer, _board_pointer, _node_type, _depth, cache_table, pointer,
								 (int)_action, _next_pointer, _value, VCT_SHARED_INDEX);

		_next_pointer = _pointer;

		VCT_ACTION_INDEX += VCT_SHARED_TABLE[2 * VCT_SHARED_INDEX + 1];
		VCT_SHARED_INDEX++;
	}
	first_child = _pointer;
	release_board(board_pointer);
	expanded = true;
}

int Node::update(std::unordered_map<U64, bool> &cache_table)
{
	int tmp_proof = proof, tmp_disproof = disproof, _proof, _disproof;
	set_proof_and_disproof(_proof, _disproof, cache_table);
	if (parent >= 0 && (tmp_proof != _proof || tmp_disproof != _disproof))
	{
		return NODE_TABLE[parent].update(cache_table);
	}
	return pointer;
}

extern UCVEC SHARED_GOMOKU_TYPES;
extern UCVEC SHARED_DIRECTIONS;
extern UCVEC SHARED_ACTIONS;
extern int SHARED_INDEX;
extern int SHARED_INDEX_HEAD, SHARED_INDEX_TAIL;

int vct(BitBoard &board, int max_depth, double max_time)
{
	if (board.step < 6)
	{
		return -1;
	}

	initial_tables();
	VCT_SHARED_INDEX = 0;
	VCT_ACTION_INDEX = 0;

	int board_pointer = allocate_board();
	BOARD_TABLE[board_pointer] = board;

	std::unordered_map<U64, bool> cache_table;
	int current = board.player;
	VCT_SHARED_TABLE[1] = 0;
	int value = BOARD_TABLE[board_pointer].evaluate(VCT_SHARED_ACTIONS, 0, VCT_SHARED_TABLE[1], 
													current, cache_table, -1);

	if (value >= 0)
	{
		if (value)
		{
			return (int)VCT_SHARED_ACTIONS[0];
		}
		else
		{
			return -1;
		}
	}

	if (BLACK_VCT_TABLE.size() > VCT_TABLE_LIMIT)
	{
		BLACK_VCT_TABLE.clear();
	}
	if (WHITE_VCT_TABLE.size() > VCT_TABLE_LIMIT)
	{
		WHITE_VCT_TABLE.clear();
	}

	VCT_SHARED_TABLE[0] = 0;
	int pointer = allocate_node(), unknown; 
	Node &root = NODE_TABLE[pointer];
	root.set(pointer, board_pointer, OR, 0, cache_table, -1, -1, -1, value, 0);
	
	VCT_ACTION_INDEX += VCT_SHARED_TABLE[1];
	VCT_SHARED_INDEX++;
	clock_t start = clock();
	while (root.proof != 0 && root.disproof != 0 &&
		   (double)(clock() - start) / CLOCKS_PER_SEC < max_time)
	{
		while (NODE_TABLE[pointer].selected_node != -1)
		{
			pointer = NODE_TABLE[pointer].selected_node;
		}

		Node &node = NODE_TABLE[pointer];
		unknown = node.depth + 1 < max_depth ? -1 : 0;
		node.develop(current, unknown, cache_table);
		pointer = node.update(cache_table);
	}

	#ifdef VCT_TEST
	printf("node pointer: %d\n", NODE_POINTER);
	printf("size of black vct table: %d size of white vct table: %d\n",
		   (int)BLACK_VCT_TABLE.size(), (int)WHITE_VCT_TABLE.size());
	#endif

	if (root.proof == 0)
	{
		return NODE_TABLE[root.selected_node].action;
	}

	return -1;
}