import numpy as np
import matplotlib.pyplot as plt
from .global_constants import *
from .common import *
from .cpp import CPPBoard

plt.ion()

class Board(CPPBoard):
    def __init__(self, history=[], visualization=False):
        super(Board, self).__init__()

        self.legal_actions = 2**(BOARD_SIZE**2)
        legality = self.check_legality(history)
        if not all(legality):
            raise Exception('illegal actions: {}'.format(
                            [history[i] for i, lg in enumerate(legality) if not lg]))

        self.visualization = visualization
        self.figure = None

        for action in history:
            self.move(action, False)

    def move(self, action, check_legality=True):
        if check_legality and not self.check_legality(action):
            raise Exception('illegal action: {}'.format(action))
        if self.is_over:
            raise Exception('the game has been finished')
        self.cpp_board.move(flatten(action))
        self.legal_actions ^= BINARY_HEPLERS[flatten(action)]

    def check_legality(self, actions):
        actions = tolist(actions)
        if len(actions) == 0:
            return []
        legal_actions = bin(self.legal_actions)[2:]
        legality = []
        action_set = set()
        for action in actions:
            act = flatten(action)
            if act in action_set or legal_actions[act] == '1':
                legality.append(False)
            else:
                legality.append(True)
                action_set.add(act)
        return tosingleton(legality)

    def copy(self):
        new_board = super(Board, self).copy()
        new_board.legal_actions = self.legal_actions
        return new_board

    def visualize(self):


    # def make_figure(self):
    #         axis = np.linspace(1, SIZE+1, SIZE+1)
    #         figure = plt.figure()
    #         axes = figure.add_subplot(111, autoscale_on=False)
    #         axes.patch.set_facecolor((255 / 256., 206 / 256., 118 / 256.))
    #         axes.grid(color='k', linestyle='-', linewidth=0.2, alpha=1.)
    #         axes.set_xticks(axis)
    #         axes.set_yticks(axis)
    #         for color, c in [(BLACK, 'k'), (WHITE, 'w')]:
    #             row, col = np.where(self.planes[color] == 1.0)
    #             axes.scatter(row + 1, col + 1, c=c,
    #                          s=200, alpha=0.9, marker='o')
    #         axes.set_xticks(axis)
    #         axes.set_yticks(axis)
    #         self.figure = figure
    #         self.axes = axes
