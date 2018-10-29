import numpy as np
from AlphaGomoku.board import *
rng = np.random

board = Board([], True, visualization_time=0.2)
# history = [(7, 7), (8, 8), (6, 6), (7, 8), (8, 7)]
# for action in history:
#     board.move(action)

while not board.is_over:
    legal_actions = board.get_legal_actions()
    action = legal_actions[rng.randint(len(legal_actions))]
    board.move(action)
