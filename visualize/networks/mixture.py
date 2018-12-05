from AlphaGomoku.global_constants import *
from AlphaGomoku.common import *
from AlphaGomoku.mcts.evaluation_mcts import EvaluationMCTSBase
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.board import Board
from AlphaGomoku.utils.visualization_utils import visualize

mixture = get_network('mixture', 'resnet', 'keras', stack_nb=2)
mixture.load_weights(get_weight_file('pre', mixture.network.name, 0))
board = Board(toTensor=True, visualization=False)

def visualize_policy():
    legal_actions = board.get_legal_actions()
    distributions = mixture.predict_actions(board, sample=False)
    probs = [distributions[flatten(action)] for action in legal_actions]
    s = sum(probs)
    probs = [p / s for p in probs]
    pairs = list(zip(legal_actions, probs))
    pairs = [(act, prob) for act, prob in pairs if prob >= .01]
    board.visualize(time=.1, cla=False)
    visualize(pairs, time=2, cla=True)

def visualize_value():
    legal_actions = board.get_legal_actions()
    distributions = mixture.predict_actions(board, sample=False)
    probs = [distributions[flatten(action)] for action in legal_actions]
    s = sum(probs)
    probs = [p / s for p in probs]
    actions = [action for action, prob in zip(legal_actions, probs) if prob >= .01]
    boards = []
    for action in actions:
        copy_board = board.copy()
        copy_board.move(action)
        boards.append(copy_board)
    values = mixture.predict_values(boards)
    values = tolist(values)
    pairs = list(zip(actions, values))
    board.visualize(time=.1, cla=False)
    visualize(pairs, time=5, cla=True)

while not board.is_over:
    visualize_policy()
    # visualize_value()
    action = mixture.predict_actions(board)
    board.move(action)
visualize_policy()
