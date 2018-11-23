import numpy as np
import time
from AlphaGomoku.global_constants import *
from AlphaGomoku.cpp import CPPBoard
history = [
    (7, 7), (6, 7), (6, 8), (7, 8), (6, 6), (6, 5),
    (5, 6), (4, 6), (5, 5), (4, 4), (7, 6), (5, 9),
    (8, 6), (9, 6), (8, 7), (8, 5)
]

board = CPPBoard(history=history)

idx = 0
# for _ in range(8):
while not board.is_over:
    idx += 1
    if idx % 2 == 1:
        actions = board.get_actions(True, 1) + \
                  board.get_actions(True, 2)
        if len(actions):
            action = actions[0]
        else:
            start = time.time()
            action = board.vct(14, 3600)
            print('time: {:.4f}'.format(time.time() - start))
    else:
        actions = board.get_actions(True, OPEN_FOUR) + board.get_actions(True, FOUR)
        if len(actions) == 0:
            actions = board.get_actions(False, OPEN_FOUR) + board.get_actions(False, FOUR)
        if len(actions) == 0:
            actions = board.get_actions(True, OPEN_THREE)
        if len(actions) == 0:
            actions = board.get_actions(False, OPEN_THREE)
        if len(actions) == 0:
            print('trivial case')
            break
        actions = list(set(actions))
        action = actions[np.random.randint(len(actions))]

    board.move(action)
    print(action)
    print(board)
    time.sleep(1)
