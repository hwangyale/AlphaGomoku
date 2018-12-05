import threading
import queue
import numpy as np
from ..global_constants import *
from ..common import *
from ..utils.thread_utils import CONDITION
from ..utils.zobrist_utils import get_zobrist_key
from ..utils.generic_utils import ProgBar
from .base import MCTSBoard, Node

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

    def traverse(self, board, root, start=True):
        condition = self.condition

        traversalQueue = queue.Queue()
        updateQueue = queue.Queue()
        for index in range(self.traverse_time):
            traversalQueue.put_nowait((index, board.copy()))

        for _ in range(self.thread_number):
            thread = EvaluationTraversal(traversalQueue, updateQueue, root,
                                         self.get_virtual_value_function, self.c_puct,
                                         self.condition, start)
            self.threads.append(thread)
        return updateQueue

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
        board = MCTSBoard(board, True)
        traverse_time = self.traverse_time
        node_pool = self.node_pool
        left_pool = self.left_pool
        condition = self.condition

        Node(board.zobristKey, node_pool, left_pool,
             self.delete_threshold, self.tuple_table, self.action_table,
             self.value_table)
        root = self.node_pool[board.zobristKey]
        root.estimate(board)

        if self.value_table[board.zobristKey] is not None:
            actions = self.action_table[board.zobristKey]
            return actions[np.random.randint(len(actions))]

        if verbose:
            progbar = ProgBar(traverse_time)
            progbar.initialize()
        else:
            progbar = None
        total_count = 0
        updateQueue = self.traverse(board, root, True)
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

    def get_config(self):
        config = {
            'traverse_time': self.traverse_time,
            'c_puct': self.c_puct,
            'thread_time': self.thread_number,
            'delete_threshold': self.delete_threshold,
            'condition': None
        }

        keys2nodeConfigs = {key: node.get_config()
                            for key, node in self.node_pool.items()}
        config['keys2nodeConfigs'] = keys2nodeConfigs

        config['keys2tuples'] = {key: tpl for key, tpl in self.tuple_table.items()}
        config['keys2actions'] = {key: action for key, action in self.action_table.items()}
        config['keys2values'] = {key: value for key, value in self.value_table.items()}

        return config

    @classmethod
    def from_config(cls, config, mixture_network):
        tree = cls(
            mixture_network,
            traverse_time=config['traverse_time'],
            c_puct=config['c_puct'],
            thread_number=config['thread_number'],
            delete_threshold=config['delete_threshold'],
            condition=config['condition']
        )

        node_pool = tree.node_pool
        left_pool = tree.left_pool
        delete_threshold = tree.delete_threshold
        tuple_table = tree.tuple_table
        action_table = tree.action_table
        value_table = tree.value_table

        for key, nodeConfig in config['keys2nodeConfigs'].items():
            Node.from_config(nodeConfig, key, node_pool,
                             left_pool, delete_threshold,
                             tuple_table, action_table, value_table)

        for key, tpl in config['keys2tuples'].items():
            tuple_table[key] = tpl

        for key, actions in config['keys2actions']:
            action_table[key] = actions

        for key, value in config['keys2values']:
            value_table[key] = value

        return tree
