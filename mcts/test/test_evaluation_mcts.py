from AlphaGomoku.mcts.evaluation_mcts import EvaluationMCTS
from AlphaGomoku.mcts.rl_evaluation_mcts import RLEvaluationMCTS
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.board import Board
from AlphaGomoku.play import Human, Game

mixture = get_network('mixture', 'resnet', 'keras', stack_nb=2)
# mixture.load_weights(get_weight_file('pre', mixture.network.name, 0))
mixture.load_weights(get_weight_file('zero', mixture.network.name, 2))
mcts = EvaluationMCTS(mixture, 500, thread_number=1, delete_threshold=10)
# mcts = RLEvaluationMCTS(mixture, 500, thread_number=4, delete_threshold=10)

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
