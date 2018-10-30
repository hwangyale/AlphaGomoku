import numpy as np
from AlphaGomoku.board import *
rng = np.random

board = Board([], True, visualization_time=0.2)

while not board.is_over:
    legal_actions = board.get_potential_actions()
    action = legal_actions[rng.randint(len(legal_actions))]
    board.move(action)
