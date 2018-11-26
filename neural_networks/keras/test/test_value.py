import time
import numpy as np
from AlphaGomoku.global_constants import *
from AlphaGomoku.board import Board
from AlphaGomoku.neural_networks.keras.value import *
from AlphaGomoku.utils.generic_utils import ProgBar

value = ResNetValue(stack_nb=1, board_cls=Board, weight_decay=0.0005)
value.predict(Board(toTensor=True))

N = 100
batch_size = 1000
total_time = 0.0
progbar = ProgBar(N * batch_size)
progbar.initialize()
for t in range(N):
    boards = []
    while len(boards) < batch_size:
        board = Board(toTensor=True)
        step = np.random.randint(BOARD_SIZE**2 // 2) + 1
        for _ in range(step):
            legal_actions = board.get_legal_actions()
            action = legal_actions[np.random.randint(len(legal_actions))]
            board.move(action)
            if board.is_over:
                break
        if not board.is_over:
            boards.append(board)
            progbar.update()
    start = time.time()
    value.predict_values(boards)
    total_time += time.time() - start

print('average time of predicting a value: {:.4f}'.format(total_time / N / batch_size))
