from AlphaGomoku.global_constants import *
from AlphaGomoku.common import *
from AlphaGomoku.mcts.evaluation_mcts import EvaluationMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.board import Board
from AlphaGomoku.utils.visualization_utils import visualize

policy = get_network('policy', 'unitized_resnet', 'keras', stack_nb=1)
policy.load_weights(get_weight_file('pre', policy.network.name, 0))
board = Board(toTensor=True, visualization=False)

def visualize_policy():
    legal_actions = board.get_legal_actions()
    distributions = policy.predict_actions(board, sample=False)
    probs = [distributions[flatten(action)] for action in legal_actions]
    s = sum(probs)
    probs = [p / s for p in probs]
    pairs = list(zip(legal_actions, probs))
    pairs = [(act, prob) for act, prob in pairs if prob >= .01]
    board.visualize(time=.1, cla=False)
    visualize(pairs, time=2, cla=True)

while not board.is_over:
    visualize_policy()
    action = policy.predict_actions(board)
    board.move(action)
visualize_policy()
