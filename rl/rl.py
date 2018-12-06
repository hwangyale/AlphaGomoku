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
        boards = [board_cls() for _ in range(number)]
        indice = list(range(number))
        play_indice = [:self.batch_size]
        step = 0
        finished = 0

    def run_from_config(self, config):


def evaluator():
    pass

def generator():
    pass

def trainer():
    pass

def main_loop():
    pass
