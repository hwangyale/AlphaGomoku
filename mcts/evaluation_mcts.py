import threading
import queue
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..utils.thread_utils import CONDITION
from ..utils.zobrist_utils import get_zobrist_key
from ..utils.generic_utils import ProgBar
from .base import MCTSBoard, Node, MCTSBase

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


class EvaluationMCTS(MCTSBase):
    def __init__(self, mixture_network, *args, **kwargs):
        self.mixture_network = mixture_network
        super(EvaluationMCTS, self).__init__(*args, **kwargs)

    def traverse(self, board, root, traversalQueue,
                 updateQueue, start=True):
        condition = self.condition

        for index in range(self.traverse_time):
            traversalQueue.put_nowait((index, board.copy()))

        threads = []
        for _ in range(self.thread_number):
            thread = EvaluationTraversal(traversalQueue, updateQueue, root,
                                         self.get_virtual_value_function, self.c_puct,
                                         self.condition, start)
            threads.append(thread)
        return threads

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

    def mcts(self, boards, verbose=1, node_cls=Node):
        boards = tolist(boards)
        traverse_time = self.traverse_time
        condition = self.condition
        value_table = self.value_table
        action_table = self.action_table

        roots = {}
        actions = {}
        for board in boards:
            node_pool = self.get_node_pool(board)
            node_cls(board.zobristKey,
                     node_pool,
                     self.get_left_pool(board),
                     self.delete_threshold, self.tuple_table,
                     self.action_table, self.value_table)
            root = node_pool[board.zobristKey]
            root.estimate(MCTSBoard(board, True))
            if value_table[board.zobristKey] is not None:
                actions[board] = self.process_root(board, root)
            else:
                roots[board] = root

        if len(roots) == 0:
            return tosingleton([actions[board] for board in boards])

        traverse_boards = list(roots.keys())
        max_count = len(traverse_boards) * traverse_time

        if verbose:
            progbar = ProgBar(max_count)
            progbar.initialize()
        else:
            progbar = None
        total_count = 0
        traversalQueue = queue.Queue()
        updateQueue = queue.Queue()
        threads = []
        for board in traverse_boards:
            threads += self.traverse(MCTSBoard(board, True), roots[board], traversalQueue,
                                     updateQueue, False)
        for thread in threads:
            thread.start()
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
            if total_count == max_count:
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
            if total_count == max_count:
                break

        for board in traverse_boards:
            node_pool = self.get_node_pool(board)
            left_pool = self.get_left_pool(board)
            actions[board] = self.process_root(board, root)
            pairs = [(key, node_pool[key]) for key in left_pool]
            left_pool.clear()
            node_pool.clear()
            for key, node in pairs:
                node.reset()
                node_pool[key] = node

        return tosingleton([actions[board] for board in boards])

    def process_root(self, board, root):
        if root.value is not None:
            actions = self.action_table[root.zobristKey]
            return actions[np.random.randint(len(actions))]
        zobristKey = root.zobristKey
        tuples = self.tuple_table[zobristKey]
        max_action = None
        max_visit = 0.0
        node_pool = self.get_node_pool(board)
        for key, action, _ in tuples:
            node = node_pool[key]
            if node.N_r > max_visit:
                max_visit = node.N_r
                max_action = action
        return max_action

    def get_virtual_value_function(self, index):
        return MCTS_VIRTUAL_LOSS, MCTS_VIRTUAL_VISIT

    @classmethod
    def from_config(cls, *args, **kwargs):
        return super(EvaluationMCTS, cls).from_config(*args, **kwargs)
