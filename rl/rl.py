import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board


rng = np.random


class SelfPlay(object):
    def __init__(self, mcts, number, batch_size):
        self.mcts = mcts
        self.number = number
        self.batch_size = batch_size

    def run(self, cache_step=None, board_cls=Board):
        number = self.number
        batch_size = self.batch_size
        boards = [board_cls() for _ in range(number)]
        index = min(batch_size, number)
        play_indice = set(range(index))
        step = 0
        while len(play_indice) > 0:
            temp_indice = list(play_indice)
            temp_boards = [boards[idx] for idx in temp_indice]
            actions = tolist(mcts.mcts(temp_boards, verbose=0))
            for idx, board, action in zip(temp_indice, temp_boards, actions):
                board.move(action)
                if board.is_over:
                    play_indice.remove(idx)
                    if index < number:
                        play_indice.add(index)
                        index += 1
            step += 1
        


    def run_from_config(self, config):


def evaluator():
    pass

def generator():
    pass

def trainer():
    pass

def main_loop():
    pass
