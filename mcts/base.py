import threading
import queue
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..cpp import CPPBoard
from ..utils.thread_utils import CONDITION
from ..utils.zobrist_utils import get_zobrist_key, hash_history
from ..utils.generic_utils import ProgBar


BASE_BOARD = Board(toTensor=True, visualization=False, defend=False)


class MCTSBoard(Board):
    def __init__(self, board=None, initialize=True):
        if board is None:
            board = BASE_BOARD
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
            self._zobristKey = hash_history(board.history)

    def initialize_tensor(self):
        tensors = {BLACK: np.zeros((BOARD_SIZE, BOARD_SIZE, 3),
                   dtype=FLOATX)+np.array([[[0, 0, 1]]]),
                   WHITE: np.zeros((BOARD_SIZE, BOARD_SIZE, 3),
                   dtype=FLOATX)+np.array([[[0, 0, 1]]])}

        player = BLACK
        for action in self.history:
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
        self._zobristKey = get_zobrist_key(player_map(self.player), action,
                                           self._zobristKey)

    def expand(self):
        if self.is_over:
            if self.winner != DRAW:
                return [], MCTS_LOSS_VALUE
            else:
                return [], MCTS_DRAW_VALUE

        actions = self.get_actions(True, OPEN_FOUR) + \
                  self.get_actions(True, FOUR)
        if len(actions) > 0:
            return list(set(actions)), MCTS_WIN_VALUE

        actions = self.get_actions(False, OPEN_FOUR)
        if len(actions) > 0:
            return actions, MCTS_LOSS_VALUE

        actions = self.get_actions(False, FOUR)
        if len(actions) > 0:
            return actions, None

        actions = self.get_actions(True, OPEN_THREE)
        if len(actions) > 0:
            return actions, MCTS_WIN_VALUE

        action = self.vct(MCTS_VCT_DEPTH, MCTS_VCT_TIME)
        if action is not None:
            return [action], MCTS_WIN_VALUE

        actions = self.get_actions(False, OPEN_THREE)
        if len(actions) > 0:
            return actions, None

        legal_actions = self.get_legal_actions()
        actions = [action for action in legal_actions if self.check_bound(action)]
        return actions, None

    def check_bound(self, action):
        if len(self.bounds) == 0:
            return True
        row, col = action
        bounds = self.bounds
        return bounds[0] <= row <= bounds[1] and bounds[2] <= col <= bounds[3]

    def copy(self):
        new_board = MCTSBoard(initialize=False)
        new_board.cpp_board = CPPBoard.copy(self).cpp_board

        new_board.legal_actions = self.legal_actions

        new_board.toTensor = True
        new_board.tensors = {c: t.copy() for c, t in self.tensors.items()}

        new_board.bounds = self.bounds[:]

        new_board.visualization = False
        new_board.defend = False

        new_board._zobristKey = self._zobristKey
        return new_board

    @property
    def zobristKey(self):
        return self._zobristKey


class Node(object):
    def __init__(self, zobristKey, node_pool,
                 left_pool, delete_threshold,
                 tuple_table, action_table, value_table):
        if zobristKey not in node_pool:
            node_pool[zobristKey] = self
            self.zobristKey = zobristKey
            self.node_pool = node_pool
            self.left_pool = left_pool
            self.delete_threshold = delete_threshold
            self.tuple_table = tuple_table
            self.action_table = action_table
            self.value_table = value_table
            self.initialize()
        elif node_pool[zobristKey].N_r > delete_threshold:
            left_pool.add(zobristKey)

    def initialize(self):
        self.indice2parents = {}
        self.expanded = False
        self.value = None
        self.estimated = False
        self.W_r = 0.0
        self.W_v = 0.0
        self.N_r = 0.0
        self.N_v = 0.0

    def reset(self):
        self.indice2parents = {}
        self.expanded = False

    def develop(self, board, distribution):
        if self.expanded:
            raise Exception('Develop expanded node: {}!!'.format(self))
        zobristKey = self.zobristKey
        if zobristKey not in self.tuple_table:
            actions = self.action_table[zobristKey]
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

        for key, action, prob in tuples:
            Node(key, self.node_pool, self.left_pool,
                 self.delete_threshold, self.tuple_table,
                 self.action_table, self.value_table)
        self.expanded = True

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

        if total_visit > 0.0:
            max_value = -MAX_FLOAT
            max_index = -MAX_FLOAT
            for idx in range(len(tuples)):
                node = nodes[idx]
                value = node.Q + c_puct * probs[idx] * \
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

    def estimate(self, board):
        if not self.estimated:
            if board.zobristKey not in self.value_table:
                expand_actions, value = board.expand()
                self.action_table[board.zobristKey] = expand_actions
                self.value_table[board.zobristKey] = value
            else:
                value = self.value_table[board.zobristKey]
            self.value = value
            self.estimated = True

    def update(self, value, thread_index,
               virtual_loss=None, virtual_visit=None):
        if virtual_loss is None:
            virtual_loss = MCTS_VIRTUAL_LOSS
        if virtual_visit is None:
            virtual_visit = MCTS_VIRTUAL_VISIT
        if len(self.indice2parents) > 0:
            self.W_r += value
            self.W_v += virtual_loss
            self.N_v -= virtual_visit
            parent = self.indice2parents.pop(thread_index)
            parent.update(-value, thread_index, virtual_loss, virtual_visit)

    @property
    def Q(self):
        if self.N_r + self.N_v > 0:
            return (self.W_r + self.W_v) / (self.N_r + self.N_v)
        return 0.0

    def zero_sum_exception(self, actions, probs):
        print('warning: zero sum of actions in developing the node')
        prob = 1 / float(len(actions))
        return [prob for _ in actions]


class EvaluationTraversal(threading.Thread):
    def __init__(self, traversalQueue, updateQueue,
                 root, get_virtual_value_function,
                 c_puct=None, condition=None, start=True, **kwargs):
        self.traversalQueue = traversalQueue
        self.updateQueue = updateQueue
        self.root = root
        self.get_virtual_value_function = get_virtual_value_function
        self.c_puct = MCTS_C_PUCT if c_puct is None else c_puct
        self.condition = CONDITION if condition is None else condition
        super(EvaluationTraversal, self).__init__(**kwargs)
        self.setDaemon(True)
        if start:
            self.start()

    def run(self):
        traversalQueue = self.traversalQueue
        updateQueue = self.updateQueue
        root = self.root
        get_virtual_value_function = self.get_virtual_value_function
        c_puct = self.c_puct
        condition = self.condition

        while True:
            condition.acquire()
            if traversalQueue.empty():
                condition.release()
                break
            thread_index, board = traversalQueue.get_nowait()
            virtual_loss, virtual_visit = get_virtual_value_function(thread_index)
            condition.release()

            node = root
            while True:
                condition.acquire()
                if not node.expanded:
                    updateQueue.put_nowait((thread_index, board, node))
                    condition.wait()
                    condition.release()
                    break
                node = node.select(board, thread_index, virtual_loss,
                                   virtual_visit, c_puct)
                condition.release()


class EvaluationMCTSBase(object):
    def __init__(self, mixture_network,
                 traverse_time, c_puct=None, thread_number=4,
                 delete_threshold=10, condition=None):
        self.mixture_network = mixture_network
        self.traverse_time = traverse_time
        self.c_puct = MCTS_C_PUCT if c_puct is None else c_puct
        self.thread_number = thread_number
        self.delete_threshold = delete_threshold
        self.node_pool = {}
        self.left_pool = set()
        self.tuple_table = {}
        self.action_table = {}
        self.value_table = {}
        self.condition = CONDITION if condition is None else condition
        self.threads = []

    def traverse(self, board, start=True):
        board = MCTSBoard(board, True)
        condition = self.condition

        traversalQueue = queue.Queue()
        updateQueue = queue.Queue()
        for index in range(self.traverse_time):
            traversalQueue.put_nowait((index, board.copy()))

        Node(board.zobristKey, self.node_pool, self.left_pool,
             self.delete_threshold, self.tuple_table, self.action_table,
             self.value_table)
        root = self.node_pool[board.zobristKey]
        root.estimate(board)

        for _ in range(self.thread_number):
            thread = EvaluationTraversal(traversalQueue, updateQueue, root,
                                         self.get_virtual_value_function, self.c_puct,
                                         self.condition, start)
            self.threads.append(thread)
        return root, updateQueue

    def update_nowait(self, tuples, progbar=None):
        left_tuples = []
        for index, board, node in tuples:
            if node.value is not None:
                virtual_loss, virtual_visit = self.get_virtual_value_function(index)
                node.update(-node.value, index,
                            virtual_loss, virtual_visit)
                if progbar is not None:
                    progbar.update()
                continue
            left_tuples.append((index, board, node))
        return left_tuples

    def expand_and_update(self, tuples, progbar=None):
        indice = []
        boards = []
        nodes = []
        for index, board, node in tuples:
            indice.append(index)
            boards.append(board)
            nodes.append(node)
        distributions, values = self.mixture_network.predict_pairs(boards, sample=False)
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

    def mcts(self, board, verbose=1):
        traverse_time = self.traverse_time
        node_pool = self.node_pool
        left_pool = self.left_pool
        condition = self.condition

        if verbose:
            progbar = ProgBar(traverse_time)
            progbar.initialize()
        else:
            progbar = None
        total_count = 0
        root, updateQueue = self.traverse(board, True)
        while True:
            tuples = []
            condition.acquire()
            while not updateQueue.empty():
                tuples.append(updateQueue.get_nowait())
            if len(tuples) == 0:
                condition.release()
                continue
            left_tuples = self.update_nowait(tuples, progbar)
            total_count += len(tuples) - len(left_tuples)
            if total_count == traverse_time:
                condition.notifyAll()
                condition.release()
                break
            elif len(left_tuples) == 0:
                condition.notifyAll()
                condition.release()
                continue

            self.expand_and_update(left_tuples, progbar)
            total_count += len(left_tuples)
            condition.notifyAll()
            condition.release()
            if total_count == traverse_time:
                self.threads.clear()
                break

        action = self.process_root(root)

        pairs = [(key, node_pool[key]) for key in self.left_pool]
        self.left_pool.clear()
        self.node_pool.clear()
        for key, node in pairs:
            node.reset()
            self.node_pool[key] = node

        return action

    def process_root(self, root):
        zobristKey = root.zobristKey
        tuples = self.tuple_table[zobristKey]
        max_action = None
        max_visit = 0.0
        for key, action, _ in tuples:
            node = self.node_pool[key]
            if node.N_r > max_visit:
                max_visit = node.N_r
                max_action = action
        print('visit time: {}'.format(int(max_visit)))
        return max_action

    def get_virtual_value_function(self, index):
        return MCTS_VIRTUAL_LOSS, MCTS_VIRTUAL_VISIT
