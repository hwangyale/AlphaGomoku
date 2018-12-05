from AlphaGomoku.mcts.evaluation_mcts import EvaluationMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.board import Board

mixture = get_network('mixture', 'resnet', 'keras', stack_nb=1)
mixture.load_weights(get_weight_file('pre', mixture.network.name, 0))
mcts = EvaluationMCTSBase(mixture, 500, thread_number=4, delete_threshold=10)

board = Board(visualization=True)
while not board.is_over:
    action = mcts.mcts(board, 1)
    board.move(action)
