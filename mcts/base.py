import numpy as np
import threading
from ..global_constants import *
from ..board import Board
from ..utils.thread_utils import RLOCK, CONDITION
from ..utils.zobrist_utils import get_zobrist_key


BASE_BOARD = Board(toTensor=True, visualization=False, defend=False)


class MCTSBoard(Board):
    def __init__(self, board=BASE_BOARD, initialize=True):
        if initialize:
            copy_board = board.copy()
            self.cpp_board = copy_board.cpp_board

            self.legal_actions = copy_board.legal_actions

            self.toTensor = True
            if copy_board.toTensor:
                self.tensors = copy_board.tensors
            else:
                self.initialize_tensor()

            self.bounds = []

            self.visualization = False
            self.defend = False

    def initialize_tensor(self):
        tensors = {BLACK: np.zeros((BOARD_SIZE, BOARD_SIZE, 3),
                   dtype=FLOATX)+np.array([[[0, 0, 1]]]),
                   WHITE: np.zeros((BOARD_SIZE, BOARD_SIZE, 3),
                   dtype=FLOATX)+np.array([[[0, 0, 1]]])}

        player = BLACK
        for action in self.get_history():
            tensors[player][action+(0, )] = 1.0
            tensors[player][action+(2, )] = 0.0
            opponenet = player_map(player)
            tensors[opponenet][action+(1, )] = 1.0
            tensors[opponenet][action+(2, )] = 0.0
            player = opponenet

        self.tensors = tensors

    def move(self, action, *args, **kwargs):
        super(MCTSBoard, self).move(action, *args, **kwargs)
        row, col = action
        if len(self.bounds) > 0:
            self.bounds[0] = min(self.bounds[0], row - MCTS_BOUND)
            self.bounds[1] = max(self.bounds[1], row + MCTS_BOUND)
            self.bounds[2] = min(self.bounds[2], col - MCTS_BOUND)
            self.bounds[3] = max(self.bounds[3], col + MCTS_BOUND)
        else:
            self.bounds = [row - MCTS_BOUND, row + MCTS_BOUND,
                           col - MCTS_BOUND, col + MCTS_BOUND]

    def check_bound(self, action):
        if len(self.bounds) == 0:
            return True
        row, col = action
        bounds = self.bounds
        return bounds[0] <= row <= bounds[1] and bounds[2] <= col <= bounds[3]

    def copy(self):
        new_board = MCTSBoard(initialize=False)
        new_board.cpp_board = CPPBoard.copy().cpp_board

        new_board.legal_actions = copy_board.legal_actions

        new_board.toTensor = True
        new_board.tensors = {c: t.copy() for c, t in self.tensors.items()}

        new_board.bounds = self.bounds[:]

        new_board.visualization = False
        new_board.defend = False


class Node(object):
    def __init__(self, zobristKey, node_pool,
                 left_pool, delete_threshold,
                 distributions_pool, tuple_table):
        if zobristKey not in node_pool:
            node_pool[zobristKey] = self
            self.initialize()
            self.zobristKey = zobristKey
            self.node_pool = node_pool
            self.left_pool = left_pool
            self.delete_threshold = delete_threshold
            self.distributions_pool = distributions_pool
            self.tuple_table = tuple_table
        elif node_pool[zobristKey].N > delete_threshold:
            left_pool.add(zobristKey)

    def initialize(self):
        self.indice2parents = {}
        self.expanded = False
        self.vct = 0
        self.W = 0.0
        self.N = 0.0

    def reset(self):
        self.indice2parents = {}
        self.expanded = False

    def develop(self, board):
        if self.expanded:
            raise Exception('Develop expanded node!!')
        zobristKey = self.zobristKey
        if zobristKey not in self.tuple_table:
            actions = [action for action in board.get_legal_actions()
                       if board.check_bound(action)]
            distribution = self.distributions_pool[zobristKey]
            probs = [distribution[flatten(action)] for action in actions]
            s = sum(probs)
            if s <= 0.0:
                probs = self.zero_sum_exception(actions, probs)
            else:
                probs = [prob / s for prob in probs]
            color = board.player
            keys = [get_zobrist_key(color, action, zobristKey)
                    for action in actions]
            tuples = list(zip(keys, actions, probs))
            self.tuple_table[zobristKey] = tuples
        else:
            tuples = self.tuple_table[zobristKey]

        for key, action, prob in tuples
            Node(key, self.node_pool, self.left_pool,
                 self.delete_threshold, self.distributions_pool,
                 self.tuple_table)

    def select(self, board, thread_index,
               virtual_loss=MCTS_VIRTUAL_LOSS,
               virtual_visit=MCTS_VIRTUAL_VISIT,
               c_puct=MCTS_C_PUCT):

        if self.vct == 0:
            vct_action = board.vct(MCTS_VCT_DEPTH, MCTS_VCT_TIME)
            self.vct = -1 if vct_action is None else 1
        if self.vct == 1:
            return MCTS_GET_VCT

        tuples = self.tuple_table[self.zobristKey]
        nodes = []
        Qs = []
        Ns = []
        actions = []
        probs = []
        total_visit = 0.0
        for key, action, prob in tuples:
            node = self.node_pool[key]
            nodes.append(node)
            total_visit += node.N
            actions.append(action)
            probs.append(prob)

        if total_visit > 0.0:
            max_value = -MAX_FLOAT
            max_index = -MAX_FLOAT
            for idx in range(len(tuples)):
                node = nodes[idx]
                value = node.Q + c_puct * probs[idx] * \
                        total_visit**0.5 / (1 + node.N)
                if value > max_value:
                    max_value = value
                    max_index = idx
        else:
            max_index = max(list(range(len(tuples))),
                            key=lambda idx: probs[idx])

        max_action = actions[max_index]
        max_node = nodes[max_index]
        max_node.N += 1
        max_node.indice2parents[thread_index] = self
        max_node.W -= virtual_loss
        max_node.N += virtual_visit
        return max_action

    def update(self, value, thread_index,
               virtual_loss=MCTS_VIRTUAL_LOSS,
               virtual_visit=MCTS_VIRTUAL_VISIT):
        if len(self.indice2parents) > 0:
            self.W += value
            self.W += virtual_loss
            self.N -= virtual_visit
            parent = self.indice2parents.pop(thread_index)
            parent.update(-value, thread_index, virtual_loss, virtual_visit)

    @property
    def Q(self):
        if self.N > 0:
            return self.W / self.N
        return 0.0
