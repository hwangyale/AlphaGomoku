import numpy as np
import matplotlib.pyplot as plt
from AlphaGomoku.board import *
rng = np.random

board = Board([], True, True, visualization_time=1)

while not board.is_over:
    legal_actions = board.get_potential_actions()
    action = legal_actions[rng.randint(len(legal_actions))]
    board.move(action)
board.visualize(60)
