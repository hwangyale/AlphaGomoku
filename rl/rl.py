import os
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..mcts import get as mcts_get
from ..neural_networks.keras import get as network_get
from ..neural_networks.weights import get_config_file, get_weight_file
from ..utils.json_utils import json_dump_tuple, json_load_tuple


rng = np.random


class SelfPlayBase(object):
    def __init__(self, mcts, number=1000, batch_size=10, initialize=True):
        self.mcts = mcts
        self.number = number
        self.batch_size = batch_size
        if initialize:
            index = min(batch_size, number)
            self.index = index
            self.boards = set(Board() for _ in range(index))
            self.step = 0
            self.finished = 0

    def generator(self):
        mcts = self.mcts
        number = self.number
        batch_size = self.batch_size
        boards = self.boards
        while len(boards) > 0:
            yield self.step, self.finished
            temp_boards = list(boards)
            actions = tolist(mcts.mcts(temp_boards, verbose=0))
            for board, action in zip(temp_boards, actions):
                board.move(action)
                if board.is_over:
                    boards.remove(board)
                    self.finished += 1
                    if self.index < number:
                        boards.add(Board())
                        self.index += 1
            self.step += 1
        self.raw_samples = self.get_samples_from_mcts(mcts)
        yield self.step, self.finished

    def get_samples_from_mcts(self):
        raise NotImplementedError('`get_samples_from_mcts` has not '
                                  'been implemented')

    def get_tensor_from_history(self, history):
        tensor = np.zeros((BOARD_SIZE, BOARD_SIZE, 3), dtype=FLOATX) \
                 +np.array([[[0, 0, 1]]])
        player = BLACK if len(history) % 2 == 0 else WHITE
        color = BLACK
        for action in history:
            if color == player:
                tensor[action+(0, )] = 1.0
            else:
                tensor[action+(1, )] = 1.0
            tensor[action+(2, )] = 0.0
            color = player_map(color)
        return tensor

    def get_tensor_from_visits(self, visits, get_max):
        tensor = np.zeros((BOARD_SIZE, BOARD_SIZE), dtype=FLOATX)
        if get_max:
            max_action = None
            max_visit = 0
            for action, visit in visits:
                if visit > max_visit:
                    max_action = action
                    max_visit = visit
            tensor[max_action] = 1.0
        else:
            s = float(sum([visit for _, visit in visits]))
            for action, visit in visits:
                tensor[action] = visit / s
        return tensor

    def get_samples(self):
        if not hasattr(self, 'raw_samples'):
            raise Exception('Self-play has not been finished')
        return self.process_raw_samples(self.raw_samples)

    def process_raw_samples(self, raw_samples):
        raise NotImplemented('`process_raw_samples` has not been implemented')

    def get_config(self):
        config = [
            'mcts': {
                'class_name': self.mcts.__class__.__name__,
                'config': self.mcts.get_config()
            },
            'number': self.number,
            'batch_size': self.batch_size,
            'index': self.index,
            'step': self.step,
            'finished': self.finished
        ]
        if hasattr(self, 'raw_samples'):
            config['raw_samples'] = tuples_to_json(self.raw_samples)
        return config

    @classmethod
    def from_config(cls, config):
        mcts, boards = mcts_get(config.pop('mcts'))
        raw_samples = config.pop('raw_samples', None)
        sf = cls(mcts, initialize=False)
        sf.__dict__.update(config)
        sf.boards = set(board for board in boards if not board.is_over)
        if raw_samples is not None:
            sf.raw_samples = raw_samples
        return sf


class EvaluationSelfPlay(SelfPlayBase):
    def get_samples_from_mcts(self, mcts):
        tuples = []
        player_dict = {0: BLACK, 1: WHITE}
        for board, index in mcts.board_indice.items():
            winner = board.winner
            for history, visits in mcts.visit_container:
                player = player_dict[len(history) % 2]
                if winner == DRAW:
                    value = MCTS_DRAW_VALUE
                elif winner == player:
                    value = MCTS_WIN_VALUE
                else:
                    value = MCTS_LOSS_VALUE
                tuples.append((history, visits, value))
        return tuples

    def process_raw_samples(self, raw_samples):
        board_tensors = []
        action_tensors = []
        values = []
        for history, visits, value in raw_samples:
            board_tensor.append(self.get_tensor_from_history(history))
            action_tensors.append(
                self.get_tensor_from_visits(visits, len(history)>=MCTS_RL_SAMPLE_STEP)
            )
            values.append(value)
        return board_tensors, action_tensors, values

    def get_config(self, index):
        config = super(EvaluationSelfPlay, self).get_config()
        mixture = self.mcts.mixture
        config['mixture'] = {
            'class_name': mixture.__class__.__name__,
            'config': mixture.get_config(True, 'rl', index)
        }
        return config

    @classmethod
    def from_config(cls, config):
        mixture = network_get(config.pop('mixture'))
        sf = super(EvaluationSelfPlay, cls).from_config(config)
        sf.mcts.mixture = mixture
        return sf


class Evaluator(object):
    def __init__(self, best_mcts, new_mcts, boards_each_side):
        self.mcts = [best_mcts, new_mcts]
        self.boards_each_side = boards_each_side

    def generator(self):
        mcts = self.mcts
        boards = set()
        for ply in [0, 1]:
            for _ in range(self.boards_each_side):
                boards.add((board, ply))
        step = 0
        finished = 0
        player = 0
        win = 0
        loss = 0
        draw = 0
        while len(boards) > 0:
            yield step, finished, win, loss, draw
            temp_boards = [board for board, ply in boards if ply == player]
            actions = tolist(mcts[player].mcts(temp_boards))
            for board, action in zip(temp_boards, actions)
                board.move(action)
                boards.remove((board, player))
                if board.is_over:
                    finished += 1
                    if board.winner == DRAW:
                        draw += 1
                    elif player == 1:
                        win += 1
                    else:
                        loss += 1
                else:
                    boards.add((board, player ^ 1))
            player ^= 1
        step += 1
        yield step, finished, win, loss, draw


class TrainerGenerator(object):
    def __init__(self, )

def main_loop():
    pass
