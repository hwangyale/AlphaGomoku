import threading
import queue
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..utils.thread_utils import CONDITION
from ..utils.zobrist_utils import get_zobrist_key
from ..utils.generic_utils import ProgBar
from .base import MCTSBoard, Node
from .evaluation_mcts import EvaluationTraversal, EvaluationMCTS


class RLNode(Node):
    def select(self, board, thread_index, virtual_loss=None,
               virtual_visit=None, c_puct=None):
        if virtual_loss is None:
            virtual_loss = MCTS_VIRTUAL_LOSS
        if virtual_visit is None:
            virtual_visit = MCTS_VIRTUAL_VISIT
        if c_puct is None:
            c_puct = MCTS_C_PUCT

        tuples = self.tuple_table[self.zobristKey]
        nodes = []
        actions = []
        probs = []
        total_visit = 0.0
        for key, action, prob in tuples:
            node = self.node_pool[key]
            total_visit += node.N_r + node.N_v
            nodes.append(node)
            actions.append(action)
            probs.append(prob)

        if len(self.indice2parents) == 0 and not hasattr(self, 'dirichlet_noise'):
            self.dirichlet_noise = np.random.dirichlet_noise(
                [MCTS_DIRICHLET_NOISE]*len(tuples), 1
            )[0, ...]

        if total_visit > 0.0:
            max_value = -MAX_FLOAT
            max_index = -MAX_FLOAT
            for idx in range(len(tuples)):
                node = nodes[idx]
                if len(self.indice2parents) == 0:
                    prob = (1 - MCTS_DIRICHLET_WEIGHT) * probs[idx] + \
                           MCTS_DIRICHLET_WEIGHT * self.dirichlet_noise[idx]
                else:
                    prob = probs[idx]
                value = node.Q + c_puct * prob * \
                        total_visit**0.5 / (1 + node.N_r + node.N_v)
                if value > max_value:
                    max_value = value
                    max_index = idx
        else:
            max_index = max(list(range(len(tuples))),
                            key=lambda idx: probs[idx])

        max_action = actions[max_index]
        max_node = nodes[max_index]
        if max_node.N_r == max_node.delete_threshold:
            max_node.left_pool.add(max_node.zobristKey)
        max_node.N_r += 1
        max_node.indice2parents[thread_index] = self
        max_node.W_v -= virtual_loss
        max_node.N_v += virtual_visit
        board.move(max_action)
        max_node.estimate(board)
        return max_node


class RLEvaluationMCTS(EvaluationMCTS):
    def __init__(self, *args, **kwargs):
        super(RLEvaluationMCTS, self).__init__(*args, **kwargs)
        self.visit_container = []

    def expand_and_update(self, tuples, progbar=None):
        indice = []
        boards = []
        nodes = []
        for index, board, node in tuples:
            indice.append(index)
            boards.append(board)
            nodes.append(node)
        distributions, values = \
            self.mixture_network.predict_pairs(boards, sample=False, transform=True)
        distributions = tolist(distributions)
        values = tolist(values)

        for idx, node in enumerate(nodes):
            if not node.expanded:
                node.develop(boards[idx], distributions[idx])
            index = indice[idx]
            virtual_loss, virtual_visit = self.get_virtual_value_function(index)
            node.update(-values[idx], index, virtual_loss, virtual_visit)
            if progbar is not None:
                progbar.update()

    def process_root(self, root, node_pool):
        zobristKey = root.zobristKey
        tuples = self.tuple_table[zobristKey]
        max_action = None
        max_visit = 0.0
        for key, action, _ in tuples:
            node = node_pool[key]
            if node.N_r > max_visit:
                max_visit = node.N_r
                max_action = action
        return max_action
