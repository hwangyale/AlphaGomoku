import os
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..mcts import get as mcts_get
from ..neural_networks.keras import get as network_get
from ..neural_networks.keras.train import TrainerBase
from ..neural_networks.weights import get_config_file, get_weight_file
from ..utils.generic_utils import ProgBar
from ..utils.json_utils import json_dump_tuple, json_load_tuple
from ..utils.board_utils import tensor_functions
from ..utils.keras_utils import optimizer_wrapper, CacheCallback
from ..temp import get_cache_folder, remove_folder


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

    def get_raw_samples(self):
        if not hasattr(self, 'raw_samples'):
            raise Exception('Self-play has not been finished')
        return self.raw_samples

    def process_raw_samples(self, raw_samples):
        raise NotImplementedError('`process_raw_samples` has not been implemented')

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


class RLTrainerBase(TrainerBase):
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

    def get_validation_data(self):
        return None


class EvaluationTrainer(RLTrainerBase):
    def get_loss(self):
        return {'distribution': 'categorical_crossentropy',
                'value': 'mean_squared_error'}

    def get_metrics(self):
        return {'distribution': 'acc'}

    def get_loss_weights(self):
        return {'distribution': 1.0, 'value': 1.0}

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

    def get_generator(self, cache, batch_size, split):
        self.load_tuples(False, 1.0)
        raw_samples = self.tuple_container[0]
        board_tensors, action_tensors, values = self.process_raw_samples(raw_samples)
        def generator():
            number = len(raw_samples)
            batches = KE.training._make_batches(number, batch_size)
            while True:
                indice = list(range(number))
                np.random.shuffle(indice)
                for batch_begin, batch_end in batches:
                    Xs = []
                    Ys = []
                    Zs = np.zeros((batch_end-batch_begin, 1), dtype=FLOATX)
                    for idx, index in enumerate(indice[batch_begin:batch_end]):
                        board_tensor = board_tensors[index]
                        action_tensor = action_tensors[index]
                        value = values[index]
                        tensor_func = tensor_functions[np.random.randint(8)]
                        Xs.append(np.expand_dims(tensor_func(board_tensor), 0))
                        Ys.append(np.reshape(tensor_func(action_tensor), (1, -1)))
                        Zs[idx, 0] = value
                    yield np.concatenate(Xs, axis=0), [np.concatenate(Ys, axis=0), Zs]
        return generator(), (len(tuples) + batch_size - 1) // batch_size

    def get_cache_folder(self):
        get_cache_folder('rl_mixture')
        return get_cache_folder('rl_mixture\\{}'.format(self.network.name))


class MainLoopBase(object):
    def __init__(self, network, **kwargs):
        self.network = network
        defaults = {
            'self_play_mcts_config': {
                'traverse_time': 500, 'c_puct': None,
                'thread_number': 1, 'delete_threshold': 10
            },
            'self_play_number': 1000,
            'self_play_batch_size': 20,
            'self_play_cache_step': 10,
            'evaluate_number': 100,
            'win_ratio': 0.55,
            'train_batch_size': 128,
            'train_epochs': 100
        }
        defaults.update(kwargs)
        self.config = defaults
        self.state = 0
        self.index = 0

    def run(self):
        raise NotImplementedError('`run` has not been implemented')

    def copy_network(self, network):
        folder = self.get_cache_folder()
        while True:
            weight_file = os.path.join(folder, '{}.npz'.format(rng.randint(2**32)))
            if os.path.exists(weight_file):
                continue
        config = network.get_config()
        copy_network = KE.Model.from_config(config)
        network.save_weights(weight_file)
        copy_network.load_weights(weight_file)
        os.remove(weight_file)
        return copy_network

    def get_cache_folder(self):
        raise NotImplementedError('`get_cache_folder` has not been implemented')

    def get_cache_self_play_path(self):
        folder = self.get_cache_folder()
        return os.path.join(folder, 'self_play.json')


class EvaluationMainLoop(MainLoopBase):
    def run(self, load_cache):
        if load_cache:
            pass
        else:
            state = self.state
            best_network = self.network
            config = self.config
            index = self.index
        self_play_file = self.get_cache_self_play_path()
        while True:
            if state == 0:
                mcts = RLEvaluationMCTS(self.copy_network(self.best_network),
                                        **config['self_play_mcts_config'])
                sf = EvaluationSelfPlay(mcts, config['self_play_number'],
                                        config['self_play_batch_size'])
                print('self-play:')
                progbar = ProgBar(config['self_play_number'])
                pre_finished = 0
                for step, finished in sf.generator():
                    if step % config['self_play_cache_step'] == 0:
                        config = sf.get_config(index)
                        json_dump_tuple(config, self_play_file)
                        state = 1
                    progbar.update(finished-pre_finished, 'step: {}'.format(step))
                    pre_finished = finished


    def get_cache_folder(self):
        return get_cache_folder('rl_mixture')
