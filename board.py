import numpy as np
import matplotlib.pyplot as plt
from .global_constants import *
from .common import *
from .cpp import CPPBoard

plt.ion()

class Board(CPPBoard):
    def __init__(self, history=[], visualization=False, visualization_time=2):
        super(Board, self).__init__()

        self.legal_actions = 2**(BOARD_SIZE**2)
        legality = self.check_legality(history)
        if not all(legality):
            raise Exception('illegal actions: {}'.format(
                            [history[i] for i, lg in enumerate(legality) if not lg]))

        self.visualization = visualization
        self.visualization_time = visualization_time
        self.axes = None if visualization else plt.gca()

        for action in history:
            self.move(action, False)

    def move(self, action, check_legality=True):
        if check_legality and not self.check_legality(action):
            raise Exception('illegal action: {}'.format(action))
        if self.is_over:
            raise Exception('the game has been finished')
        self.cpp_board.move(flatten(action))
        self.legal_actions ^= BINARY_HEPLERS[flatten(action)]
        if self.visualization:
            self.visualize()

    def check_legality(self, actions):
        actions = tolist(actions)
        if len(actions) == 0:
            return []
        legal_actions = bin(self.legal_actions)[:2:-1]
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

    def get_legal_actions(self):
        legal_actions = bin(self.legal_actions)[:2:-1]
        return [unflatten(act) for act in range(BOARD_SIZE**2)
                if legal_actions[act] == '0']

    def copy(self):
        new_board = super(Board, self).copy()
        new_board.legal_actions = self.legal_actions
        new_board.visualization = False
        new_board.visualization_time = self.visualization_time
        new_board.axes = None
        return new_board

    def visualize(self, time=None):
        if self.axes is None:
            self.axes = plt.gca()
        axes = self.axes
        axis = np.linspace(0, BOARD_SIZE+1, BOARD_SIZE+2)
        axes.patch.set_facecolor((255 / 256., 206 / 256., 118 / 256.))
        axes.grid(color='k', linestyle='-', linewidth=0.2, alpha=1.)
        board = self.get_board()
        for row in range(BOARD_SIZE):
            for col in range(BOARD_SIZE):
                color = {BLACK: 'k', WHITE: 'w'}.get(board[row][col], None)
                if color is not None:
                    axes.scatter(row + 1, col + 1, c=color,
                                 s=200, alpha=0.9, marker='o')
        axes.set_xticks(axis)
        axes.set_yticks(axis)
        axes.set_xticklabels(['']+[str(i) for i in range(1, BOARD_SIZE+1)]+[''])
        axes.set_yticklabels(['']+[str(i) for i in range(1, BOARD_SIZE+1)]+[''])
        plt.show()
        if time is None:
            time = self.visualization_time
        plt.pause(time)
        plt.cla()
