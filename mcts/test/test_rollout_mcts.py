from AlphaGomoku.mcts.rollout_mcts import RolloutMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.board import Board

policy = get_network('policy', 'resnet', 'keras', stack_nb=1)
# policy.load_weights(get_weight_file('pre', policy.network.name, 0))
mcts = RolloutMCTSBase(policy, policy, 100, thread_number=4,
                       depth=6, delete_threshold=10)

board = Board(visualization=True)
while not board.is_over:
    action = mcts.mcts(board, 1)
    board.move(action)
