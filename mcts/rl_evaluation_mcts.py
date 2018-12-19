import threading
import queue
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..utils.thread_utils import CONDITION
from ..utils.zobrist_utils import get_zobrist_key
from ..utils.generic_utils import ProgBar
from ..process_data import tuples_to_json, json_to_tuples
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
            self.dirichlet_noise = np.random.dirichlet(
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
            if len(self.indice2parents) == 0:
                max_index = max(list(range(len(tuples))),
                                key=lambda idx:
                                (1-MCTS_DIRICHLET_WEIGHT)*probs[idx] +
                                MCTS_DIRICHLET_WEIGHT*self.dirichlet_noise[idx])
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
        self.visit_containers = []

    def mcts(self, board, verbose=1, verbose_endline=True):
        actions = super(RLEvaluationMCTS, self).mcts(board, verbose, RLNode, verbose_endline)
        return actions

    def get_visit_container(self, board):
        index = self.get_board_index(board)
        while index >= len(self.visit_containers):
            self.visit_containers.append([])
        return self.visit_containers[index]

    def process_root(self, board, root):
        history = board.history[:]
        step = len(history)
        visit_container = self.get_visit_container(board)
        if root.value is not None:
            actions = self.action_table[root.zobristKey]
            visits = [(action, 1) for action in actions]
            visit_container.append((history, visits))
            if step < MCTS_RL_SAMPLE_STEP:
                return actions[np.random.randint(len(actions))]
            else:
                return actions[0]

        if step >= MCTS_RL_SAMPLE_STEP:
            max_action = None
            max_visit = 0.0
        node_pool = self.get_node_pool(board)
        visits = []
        for key, action, _ in self.tuple_table[root.zobristKey]:
            node = node_pool[key]
            if node.N_r > 0:
                visits.append((action, int(node.N_r)))
            if step >= MCTS_RL_SAMPLE_STEP and node.N_r > max_visit:
                max_visit = node.N_r
                max_action = action
        visit_container.append((history, visits))
        if step < MCTS_RL_SAMPLE_STEP:
            Ns = [N for _, N in visits]
            s = float(sum(Ns))
            idx = multinomial_sampling([N/s for N in Ns])
            return visits[idx][0]
        else:
            return max_action

    def get_config(self):
        config = super(RLEvaluationMCTS, self).get_config()
        config['visit_containers'] = [tuples_to_json(visits)
                                      for visits in self.visit_containers]
        return config

    @classmethod
    def from_config(cls, config, *args, **kwargs):
        visit_containers = config.pop('visit_containers')
        kwargs['node_cls'] = RLNode
        tree, boards, ids = super(RLEvaluationMCTS, cls).from_config(config, *args, **kwargs)
        tree.visit_containers = [json_to_tuples(visits)
                                 for visits in visit_containers]
        return tree, boards, ids
