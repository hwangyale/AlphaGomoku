import queue
import numpy as np
from .global_constants import *
from .board import Board
from .utils.zobrist_utils import hash_history, get_zobrist_key
from .utils.board_utils import action_functions, tensor_functions
from .utils.generic_utils import ProgBar

FILTER_TIME = 2

def check_legality(history):
    board = Board(toTensor=False, visualization=False)
    def get_urgent_actions():
        if board.step < 5:
            return []
        actions = board.get_actions(True, OPEN_FOUR) + board.get_actions(True, FOUR)
        if len(actions) > 0:
            return actions

        actions = board.get_actions(False, OPEN_FOUR) + board.get_actions(False, FOUR)
        if len(actions) > 0:
            return actions

        actions = board.get_actions(True, OPEN_THREE)
        if len(actions) > 0:
            return actions

        actions = board.get_actions(False, OPEN_THREE)
        if len(actions) > 0:
            return actions + board.get_actions(True, THREE)

        return []

    for row, col, *_ in history:
        action = (row, col)
        urgent_actions = get_urgent_actions()
        if len(urgent_actions) > 0 and action not in urgent_actions:
            return False
        board.move(action, True, False)

    return board.is_over

def process_history(container, check=True):
    tuples = []
    table = set()
    progbar = ProgBar(len(container))
    progbar.initialize()
    for history in container:
        if check and not check_legality(history):
            print('ignore illegal history')
            progbar.update()
            continue
        boards = [Board(toTensor=True, visualization=False) for _ in action_functions]
        get_value_idxs = []
        for row, col, t, *_ in history:
            actions = [func((row, col)) for func in action_functions]
            if t > FILTER_TIME:
                flag = True
                for board, action in zip(boards, actions):
                    if (board.zobristKey, action) in table:
                        flag = False
                        break
                if flag:
                    get_value_idxs.append(len(tuples))
                    tuples.append((boards[0].history[:], actions[0]))
                    table.add((boards[0].zobristKey, actions[0]))
            for board, action in zip(boards, actions):
                board.move(action, check_legality=(not check))
        if not boards[0].is_over:
            raise Exception('Unfinished history: {}'.format(boards[0].history))
        winner = boards[0].winner
        for idx in get_value_idxs:
            if winner == DRAW:
                value = NETWORK_DRAW_VALUE
            else:
                history = tuples[idx][0]
                if (len(history) % 2 == 0 and winner == BLACK) or \
                    (len(history) % 2 == 1 and winner == WHITE):
                    value = NETWORK_WIN_VALUE
                else:
                    value = NETWORK_LOSS_VALUE
            tuples[idx] = tuples[idx] + (value, )

        progbar.update()

    return tuples


class TupleNode(object):
    def __init__(self):
        self.data_container = []
        self.children = {}

    def append(self, data):
        self.data_container.append(data)


def tuples_to_tree(tuples):
    nodes = [TupleNode()]
    for tpl in tuples:
        history, *data = tpl
        node_index = 0
        for action in history:
            node_index = nodes[node_index].children.setdefault(action, len(nodes))
            if node_index == len(nodes):
                nodes.append(TupleNode())
        nodes[node_index].append(data)
    return nodes

def tree_to_json(nodes):
    json_object = []
    for node in nodes:
        json_object.append((node.data_container, list(node.children.items())))
    return json_object

def json_to_tree(json_object):
    nodes = []
    for data_container, children in json_object:
        node = TupleNode()
        node.data_container = data_container
        node.children = dict(children)
        nodes.append(node)
    return nodes

def tree_to_tuples(nodes):
    tuples = []
    node_queue = queue.Queue()
    node_queue.put((nodes[0], []))
    while not node_queue.empty():
        node, history = node_queue.get()
        for data in node.data_container:
            tuples.append((history[:], )+tuple(data))
        for action, node_index in node.children.items():
            node_queue.put((nodes[node_index], history+[action]))
    return tuples

def tuples_to_json(tuples):
    return tree_to_json(tuples_to_tree(tuples))

def json_to_tuples(json_object):
    return tree_to_tuples(json_to_tree(json_object))
