from AlphaGomoku.mcts.rollout_mcts import RolloutMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.board import Board
from AlphaGomoku.play import Human, Game

policy = get_network('policy', 'resnet', 'keras', stack_nb=2)
policy.load_weights(get_weight_file('pre', policy.network.name, 0))
mcts = RolloutMCTSBase(policy, policy, 100, thread_number=4,
                       depth=6, delete_threshold=10)

mcts.get_action = lambda board: mcts.mcts(board, 1)

pattern = int(input('1: player vs ai, 2: ai vs player, 3: ai vs ai: '))
if pattern == 1:
    black = Human()
    white = mcts
elif pattern == 2:
    black = mcts
    white = Human()
else:
    black = mcts
    white = mcts

game = Game(black, white, visualization=True, visualization_time=3)
game.play()
