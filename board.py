__all__ = ['Board']
import numpy as np
import matplotlib.pyplot as plt
from .global_constants import *
from .common import *
from .cpp import CPPBoard
from .utils.zobrist_utils import get_zobrist_key


class Board(CPPBoard):
    def __init__(self, history=[],
                 toTensor=False,
                 visualization=False,
                 visualization_time=2,
                 defend=True,
                 attack_vct_depth=ATTACK_VCT_DEPTH,
                 attack_vct_time=ATTACK_VCT_TIME,
                 defend_vct_depth=DEFEND_VCT_DEPTH,
                 defend_vct_time=DEFEND_VCT_TIME):
        super(Board, self).__init__()

        self.legal_actions = 2**(BOARD_SIZE**2)
        legality = self.check_legality(history)
        if not all(tolist(legality)):
            raise Exception('illegal actions: {}'.format(
                            [history[i] for i, lg in enumerate(legality) if not lg]))

        self.toTensor = toTensor
        if toTensor:
            self.make_board_tensor()

        self.visualization = visualization
        self.visualization_time = visualization_time
        self.axes = None if visualization else plt.gca()

        self.defend = defend
        self.attack_vct_depth = attack_vct_depth
        self.attack_vct_time = attack_vct_time
        self.defend_vct_depth = defend_vct_depth
        self.defend_vct_time = defend_vct_time

        self._zobristKey = 0

        for action in history:
            self.move(action, False, False)

    def move(self, action, check_legality=True, visualization=True):
        if check_legality and not self.check_legality(action):
            raise Exception('illegal action: {}'.format(action))
        if self.is_over:
            raise Exception('the game has been finished')
        self.legal_actions ^= BINARY_HEPLERS[flatten(action)]
        if self.toTensor:
            self.make_board_tensor(action)
        self._zobristKey = get_zobrist_key(self.player, action, self._zobristKey)
        self.cpp_board.move(flatten(action))
        if self.visualization and visualization:
            self.visualize()

    def check_legality(self, actions):
        actions = tolist(actions)
        if len(actions) == 0:
            return []
        legal_actions = bin(self.legal_actions)[:2:-1]
        legality = []
        action_set = set()
        for action in actions:
            if not (0 <= action[0] < BOARD_SIZE and 0 <= action[1] < BOARD_SIZE):
                legality.append(False)
                continue
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

    def get_potential_actions(self, defend=None,
                              attack_vct_depth=None, attack_vct_time=None,
                              defend_vct_depth=None, defend_vct_time=None):
        actions = self.get_actions(True, OPEN_FOUR) + self.get_actions(True, FOUR)
        if len(actions):
            return list(set(actions))

        actions = self.get_actions(False, OPEN_FOUR) + self.get_actions(False, FOUR)
        if len(actions):
            return list(set(actions))

        actions = self.get_actions(True, OPEN_THREE)
        if len(actions):
            return actions

        actions = self.get_actions(False, OPEN_THREE)
        if len(actions):
            return actions

        if attack_vct_depth is None:
            attack_vct_depth = self.attack_vct_depth
        if attack_vct_time is None:
            attack_vct_time = self.attack_vct_time
        action = self.vct(attack_vct_depth, attack_vct_time)
        if action is not None:
            return [action]

        if defend_vct_depth is None:
            defend_vct_depth = self.defend_vct_depth
        if defend_vct_time is None:
            defend_vct_time = self.defend_vct_time
        legal_actions = self.get_legal_actions()
        actions = []
        if (defend is None and self.defend) or defend:
            for action in legal_actions:
                copy_board = self.copy()
                copy_board.move(action)
                if copy_board.vct(defend_vct_depth, defend_vct_time) is None:
                    actions.append(action)
            if len(actions) == 0:
                actions = legal_actions
        else:
            actions = legal_actions

        return actions

    def make_board_tensor(self, action=None):
        if action is None:
            self.tensors = {BLACK: np.zeros((BOARD_SIZE, BOARD_SIZE, 3),
                            dtype=FLOATX)+np.array([[[0, 0, 1]]]),
                            WHITE: np.zeros((BOARD_SIZE, BOARD_SIZE, 3),
                            dtype=FLOATX)+np.array([[[0, 0, 1]]])}
            return
        player = self.player
        self.tensors[player][action+(0, )] = 1.0
        self.tensors[player_map(player)][action+(1, )] = 1.0
        for tensor in self.tensors.values():
            tensor[action+(2, )] = 0.0

    @property
    def tensor(self):
        if not self.toTensor:
            return None
        return self.tensors[self.player]

    def copy(self):
        new_board = Board(
            history=[],
            toTensor=self.toTensor,
            visualization=False,
            visualization_time=self.visualization_time,
            defend=self.defend,
            attack_vct_depth=self.attack_vct_depth,
            attack_vct_time=self.attack_vct_time,
            defend_vct_depth=self.defend_vct_depth,
            defend_vct_time=self.defend_vct_time
        )
        new_board.cpp_board = super(Board, self).copy().cpp_board
        new_board.legal_actions = self.legal_actions

        if self.toTensor:
            new_board.tensors = {c: t.copy() for c, t in self.tensors.items()}
        else:
            new_board.tensors = None

        new_board.axes = None

        new_board._zobristKey = self._zobristKey
        return new_board

    def visualize(self, time=None, cla=True):
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
        plt.show(block=False)
        if time is None:
            time = self.visualization_time
        plt.pause(time)
        if cla:
            plt.cla()

    @property
    def zobristKey(self):
        return self._zobristKey

    def get_config(self):
        config = {
            'history': self.history,
            'toTensor': self.toTensor,
            'visualization': self.visualization,
            'visualization_time': self.visualization_time,
            'defend': self.defend,
            'attack_vct_depth': self.attack_vct_depth,
            'attack_vct_time': self.attack_vct_time,
            'defend_vct_depth': self.defend_vct_depth,
            'defend_vct_time': self.defend_vct_time
        }
        return config

    @classmethod
    def from_config(cls, config):
        return cls(**config)
