import threading
import queue
import numpy as np
from ..global_constants import *
from ..common import *
from ..utils.thread_utils import CONDITION
from ..utils.zobrist_utils import get_zobrist_key
from ..utils.generic_utils import ProgBar
from .base import MCTSBoard, Node

class RolloutTraversal(threading.Thread):
    def __init__(self, traversalQueue,
                 expandQueue, updateQueue,
                 root, get_virtual_value_function,
                 depth=None, c_puct=None, condition=None,
                 start=True, **kwargs):
        self.traversalQueue = traversalQueue
        self.expandQueue = expandQueue
        self.updateQueue = updateQueue
        self.root = root
        self.get_virtual_value_function = get_virtual_value_function
        self.depth = MCTS_TRAVERSAL_DEPTH if depth is None else depth
        self.c_puct = MCTS_C_PUCT if c_puct is None else c_puct
        self.condition = CONDITION if condition is None else condition
        super(RolloutTraversal, self).__init__(**kwargs)
        self.setDaemon(True)
        if start:
            self.start()

    def run(self):
        traversalQueue = self.traversalQueue
        expandQueue = self.expandQueue
        updateQueue = self.updateQueue
        root = self.root
        get_virtual_value_function = self.get_virtual_value_function
        max_depth = self.depth
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
            depth = 0
            while depth < max_depth:
                depth += 1
                condition.acquire()
                if node.value is not None:
                    condition.release()
                    break
                if not node.expanded:
                    expandQueue.put_nowait((board, node))
                    condition.wait()
                node = node.select(board, thread_index, virtual_loss,
                                   virtual_visit, c_puct)
                condition.release()
            condition.acquire()
            updateQueue.put_nowait((thread_index, board, node))
            condition.wait()
            condition.release()


class RolloutMCTSBase(object):
    def __init__(self, policy_network, rollout_network,
                 traverse_time, depth=None, c_puct=None,
                 thread_number=4, delete_threshold=10, condition=None):
        self.policy_network = policy_network
        self.rollout_network = rollout_network
        self.traverse_time = traverse_time
        self.depth = MCTS_TRAVERSAL_DEPTH if depth is None else depth
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
        expandQueue = queue.Queue()
        updateQueue = queue.Queue()
        for index in range(self.traverse_time):
            traversalQueue.put_nowait((index, board.copy()))

        for _ in range(self.thread_number):
            thread = RolloutTraversal(traversalQueue, expandQueue,
                                      updateQueue, root,
                                      self.get_virtual_value_function,
                                      self.depth, self.c_puct,
                                      self.condition, start)
            self.threads.append(thread)
        return expandQueue, updateQueue

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

    def expand(self, pairs):
        boards = []
        nodes = []
        for board, node in pairs:
            boards.append(board)
            nodes.append(node)
        distributions = self.policy_network.predict_actions(boards, sample=False)
        distributions = tolist(distributions)

        for board, distribution, node in zip(boards, distributions, nodes):
            if not node.expanded:
                node.develop(board, distribution)

    def update_by_rollout(self, tuples, progbar=None):
        rollout_network = self.rollout_network
        boards = [board for index, board, node in tuples]
        players = [board.player for board in boards]
        winners = [None] * len(boards)
        rollout_boards = set(boards)
        while len(rollout_boards) > 0:
            predict_indice = []
            predict_boards = []
            for idx, board in enumerate(boards):
                if board in rollout_boards:
                    if MCTS_ROLLOUT_VCT_TIME > 0 and \
                            board.vct(MCTS_ROLLOUT_VCT_DEPTH, MCTS_ROLLOUT_VCT_TIME) is not None:
                        winners[idx] = board.player
                        rollout_boards.remove(board)
                    else:
                        predict_indice.append(idx)
                        predict_boards.append(board)
            if len(predict_indice) == 0:
                break
            actions = tolist(rollout_network.predict_actions(predict_boards))
            for idx, action in zip(predict_indice, actions):
                board = boards[idx]
                board.move(action)
                if board.is_over:
                    rollout_boards.remove(board)

        for idx, (index, board, node) in enumerate(tuples):
            virtual_loss, virtual_visit = self.get_virtual_value_function(index)
            if winners[idx] is None:
                if board.winner == DRAW:
                    value = 0.0
                else:
                    player = players[idx]
                    value = -1.0 if board.winner == player else 1.0
            else:
                player = players[idx]
                value = -1.0 if winners[idx] == player else 1.0
            node.update(value, index, virtual_loss, virtual_visit)
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
        expandQueue, updateQueue = self.traverse(board, root, True)
        while True:
            pairs = []
            condition.acquire()
            while not expandQueue.empty():
                pairs.append(expandQueue.get_nowait())
            if len(pairs) > 0:
                self.expand(pairs)
            tuples = []
            while not updateQueue.empty():
                tuples.append(updateQueue.get_nowait())
            if len(tuples) == 0:
                condition.notifyAll()
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

            self.update_by_rollout(left_tuples, progbar)
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
        max_node = None
        for key, action, _ in tuples:
            node = self.node_pool[key]
            if node.N_r > max_visit:
                max_visit = node.N_r
                max_action = action
                max_node = node
        print('visit time: {} value: {:.4f}'.format(int(max_visit),
              max_node.W_r/max_visit))
        return max_action

    def get_virtual_value_function(self, index):
        return MCTS_VIRTUAL_LOSS, MCTS_VIRTUAL_VISIT

    def get_config(self):
        config = {
            'traverse_time': self.traverse_time,
            'depth': self.depth,
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
    def from_config(cls, config, policy_network, rollout_network):
        tree = cls(
            policy_network, rollout_network,
            traverse_time=config['traverse_time'],
            depth=config['depth'],
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
