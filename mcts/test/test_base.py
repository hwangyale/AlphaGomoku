from AlphaGomoku.mcts.base import EvaluationMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.board import Board

mixture = get_network('mixture', 'resnet', 'keras', stack_nb=1)
mcts = EvaluationMCTSBase(mixture, 100, thread_number=4, delete_threshold=10)

board = Board(visualization=False)
mcts.mcts(board, verbose=1)
