from AlphaGomoku.mcts.base import EvaluationMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.board import Board

mixture = get_network('mixture', 'resnet', 'keras', stack_nb=1)
mcts = EvaluationMCTSBase(mixture, 500, thread_number=4, delete_threshold=10)

board = Board(visualization=True)
while not board.is_over:
    action = mcts.mcts(board, 1)
    board.move(action)
