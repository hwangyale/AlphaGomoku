import numpy as np
import time
from AlphaGomoku.cpp import CPPBoard
history = [
    (7, 7), (6, 7), (6, 8), (7, 8), (6, 6), (6, 5),
    (5, 6), (4, 6), (5, 5), (4, 4), (7, 6), (5, 9),
    (8, 6), (9, 6), (8, 7), (8, 5)
]

board = CPPBoard(history)

idx = 0
for _ in range(8):
    idx += 1
    if idx % 2 == 1:
        poses = board.get_positions(True, 1) + \
                board.get_positions(True, 2)
        if len(poses):
            pos = poses[0]
        else:
            start = time.time()
            pos = board.vct(100, 3600)
            print('time: {:.4f}'.format(time.time() - start))
    else:
        poses = board.get_positions(False, 1) + \
                board.get_positions(False, 2)
        if len(poses) == 0:
            poses = board.get_positions(False, 3)
        if len(poses) == 0:
            break
        poses = list(set(poses))
        pos = poses[np.random.randint(len(poses))]

    board.move(pos)
    print(pos)
    print(board)
