import numpy as np
from .global_constants import *
from .board import Board
from .utils.zobrist_utils import hash_history, get_zobrist_key
from .utils.board_utils import action_functions, tensor_functions

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
    pairs = set()
    table = set()
    for history in container:
        if check and not check_legality(history):
            print('ignore illegal history')
            continue
        board = Board(toTensor=True, visualization=False)
        for row, col, t, *_ in history:
            action = (row, col)
            if t <= FILTER_TIME:
                board.move(action, check_legality=True, visualization=False)
                continue
            tensor = board.tensor
            for action_func, tensor_func in zip(action_functions, tensor_functions):
                temp_action = action_func(action)
                temp_tensor = tensor_func(tensor)
