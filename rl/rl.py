import os
import time
import numpy as np
import keras.engine as KE
from keras.callbacks import LearningRateScheduler
from ..global_constants import *
from ..common import *
from ..board import Board
from ..mcts import get as mcts_get
from ..mcts import EvaluationMCTS, RLEvaluationMCTS
from ..neural_networks.keras import get as network_get
from ..neural_networks.keras.train import TrainerBase
from ..utils.generic_utils import ProgBar
from ..utils.json_utils import json_dump_tuple, json_load_tuple
from ..utils.board_utils import tensor_functions
from ..utils.keras_utils import optimizer_wrapper, CacheCallback
from ..utils.generic_utils import serialize_object
from ..process_data import tuples_to_json, json_to_tuples
from ..temp import get_cache_folder, remove_folder
from ..temp import get_temp_weight_file, remove_temp_weight_file
from ..temp import clear_temp_weight_files


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
            for board in self.boards:
                self.mcts.get_board_index(board)
            self.step = 0
            self.finished = 0

    def generator(self):
        mcts = self.mcts
        number = self.number
        batch_size = self.batch_size
        boards = self.boards
        while len(boards) > 0:
            yield self.step, self.finished
            while self.index < number and len(boards) < batch_size:
                boards.add(Board())
                self.index += 1
            temp_boards = list(boards)
            actions = tolist(mcts.mcts(temp_boards, verbose=1))
            for board, action in zip(temp_boards, actions):
                board.move(action)
                if board.is_over:
                    boards.remove(board)
                    self.finished += 1
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

    def get_config(self):
        config = {
            'mcts': serialize_object(self.mcts),
            'number': self.number,
            'batch_size': self.batch_size,
            'index': self.index,
            'step': self.step,
            'finished': self.finished
        }
        if hasattr(self, 'raw_samples'):
            config['raw_samples'] = tuples_to_json(self.raw_samples)
        return config

    @classmethod
    def from_config(cls, config, remove_weights=True):
        mcts, boards, ids = mcts_get(config.pop('mcts'), remove_weights=remove_weights)
        raw_samples = config.pop('raw_samples', None)
        sf = cls(mcts, initialize=False)
        sf.__dict__.update(config)
        sf.boards = set(board for board in boards if not board.is_over)
        if raw_samples is not None:
            sf.raw_samples = json_to_tuples(raw_samples)
        return sf


class EvaluationSelfPlay(SelfPlayBase):
    def get_samples_from_mcts(self, mcts):
        tuples = []
        player_dict = {0: BLACK, 1: WHITE}
        for board, index in mcts.board_indice.items():
            winner = board.winner
            for history, visits in mcts.visit_containers[index]:
                player = player_dict[len(history) % 2]
                if winner == DRAW:
                    value = MCTS_DRAW_VALUE
                elif winner == player:
                    value = MCTS_WIN_VALUE
                else:
                    value = MCTS_LOSS_VALUE
                tuples.append((history, visits, value))
        return tuples


class Evaluator(object):
    def __init__(self, best_mcts, new_mcts, boards_each_side,
                 batch_size, initialize=True):
        self.mcts = [best_mcts, new_mcts]
        self.boards_each_side = boards_each_side
        self.batch_size = batch_size
        if initialize:
            self.boards = set()
            self.count = 0
            self.player = 0
            for _ in range(min(batch_size, 2*boards_each_side)):
                board = Board()
                self.boards.add((board, self.count % 2))
                self.count += 1
                self.mcts[self.player].get_board_index(board)
                self.mcts[self.player^1].get_board_index(board)
            self.step = 0
            self.finished = 0
            self.win = 0
            self.loss = 0
            self.draw = 0

    def generator(self):
        while len(self.boards) > 0:
            yield self.step, self.finished, self.win, self.loss, self.draw
            while self.count < 2 * self.boards_each_side and len(self.boards) < self.batch_size:
                board = Board()
                self.boards.add((board, self.count % 2))
                self.count += 1
                self.mcts[self.player].get_board_index(board)
                self.mcts[self.player^1].get_board_index(board)
            temp_boards = [board for board, ply in self.boards if ply == self.player]
            actions = tolist(self.mcts[self.player].mcts(temp_boards, verbose=1))
            for board, action in zip(temp_boards, actions):
                board.move(action)
                self.boards.remove((board, self.player))
                if board.is_over:
                    self.finished += 1
                    if board.winner == DRAW:
                        self.draw += 1
                    elif self.player == 1:
                        self.win += 1
                    else:
                        self.loss += 1
                else:
                    self.boards.add((board, self.player ^ 1))
            self.player ^= 1
            self.step += 1
        yield self.step, self.finished, self.win, self.loss, self.draw

    def get_config(self):
        config = {
            'best_mcts': serialize_object(self.mcts[0]),
            'new_mcts': serialize_object(self.mcts[1]),
            'boards_each_side': self.boards_each_side,
            'batch_size': self.batch_size,
            'boards': [(id(board), ply) for board, ply in self.boards],
            'count': self.count, 
            'player': self.player,
            'step': self.step,
            'finished': self.finished,
            'win': self.win,
            'loss': self.loss,
            'draw': self.draw
        }
        return config

    @classmethod
    def from_config(cls, config, remove_weights=True):
        best_mcts, best_boards, best_ids = mcts_get(
            config.pop('best_mcts'), remove_weights=remove_weights)
        new_mcts, new_boards, new_ids = mcts_get(
            config.pop('new_mcts'), remove_weights=remove_weights)
        board_config = config.pop('boards')
        evaluator = cls(best_mcts, new_mcts, config.pop('boards_each_side'),
                        config.pop('batch_size'), initialize=False)
        evaluator.__dict__.update(config)

        ids2boards = {board_id: board for board_id, board in zip(best_ids, best_boards)}
        for board, board_id in zip(new_boards, new_ids):
            index = new_mcts.board_indice.pop(board)
            new_mcts.board_indice[ids2boards[board_id]] = index

        evaluator.boards = set((ids2boards[board_id], ply)
                               for board_id, ply in board_config)
        return evaluator


class RLTrainerBase(TrainerBase):
    def process_raw_samples(self, raw_samples):
        raise NotImplementedError('`process_raw_samples` has not been implemented')

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
            board_tensors.append(self.get_tensor_from_history(history))
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
        return generator(), (len(raw_samples) + batch_size - 1) // batch_size

    def get_cache_folder(self):
        get_cache_folder('rl_mixture')
        return get_cache_folder('rl_mixture\\trainer')


class MainLoopBase(object):
    def __init__(self, network=None, **kwargs):
        if network is not None:
            self.best_network = self.copy_network(network)
            self.current_network = self.copy_network(network)
        defaults = {
            'self_play_mcts_config': {
                'traverse_time': 500, 'c_puct': None,
                'thread_number': 1, 'delete_threshold': 10
            },
            'self_play_number': 1000,
            'self_play_batch_size': 20,
            'self_play_cache_step': 10,
            'evaluate_mcts_config': {
                'traverse_time': 500, 'c_puct': None,
                'thread_number': 1, 'delete_threshold': 10
            },
            'evaluate_number': 100,
            'evaluate_batch_size': 20,
            'evaluate_win_ratio': 0.55,
            'evaluate_cache_step': 10,
            'train': {
                'batch_size': 128,
                'epochs': 100,
                'verbose': 1
            }
        }
        defaults.update(kwargs)
        self.config = defaults
        self.state = 0
        self.self_play_index = 0

    def run(self):
        raise NotImplementedError('`run` has not been implemented')

    def copy_network(self, network):
        copy_network = network_get(serialize_object(network, True))
        return copy_network

    def get_cache_folder(self):
        raise NotImplementedError('`get_cache_folder` has not been implemented')

    def get_cache_self_play_path(self):
        folder = self.get_cache_folder()
        return os.path.join(folder, 'self_play_{}.json'.format(self.self_play_index))

    def get_cache_evaluate_path(self):
        folder = self.get_cache_folder()
        return os.path.join(folder, 'evaluate_{}.json'.format(self.self_play_index))

    def get_cache_self_play_sample_path(self):
        folder = self.get_cache_folder()
        return os.path.join(folder, 'raw_samples_{}.json'.format(self.self_play_index))

    def cache(self):
        folder = self.get_cache_folder()
        config = self.get_config()
        json_dump_tuple(config, os.path.join(folder, 'main_loop_config.json'))
        json_dump_tuple(serialize_object(self.best_network), os.path.join(folder, 'best_network_config.json'))
        self.best_network.save_weights(os.path.join(folder, 'best_network_weights.npz'))

    def load_cache(self):
        folder = self.get_cache_folder()
        config = json_load_tuple(os.path.join(folder, 'main_loop_config.json'))
        self.best_network = network_get(config.pop('best_network'), remove_weights=False)
        self.current_network = network_get(config.pop('current_network'), remove_weights=False)
        self.__dict__.update(config)

    def get_config(self):
        config = {
            'config': self.config,
            'best_network': serialize_object(self.best_network, True),
            'current_network': serialize_object(self.current_network, True),
            'state': self.state,
            'self_play_index': self.self_play_index
        }
        return config


def scheduler(epoch):
    if epoch <= 60:
        return 0.05
    if epoch <= 120:
        return 0.01
    if epoch <= 160:
        return 0.002
    return 0.0004


class EvaluationMainLoop(MainLoopBase):
    def run(self, load_cache):
        if load_cache:
            self.load_cache()
        config = self.config
        while True:
            if self.state == 0:
                self.self_play_index += 1
                mcts = RLEvaluationMCTS(self.copy_network(self.best_network),
                                        **config['self_play_mcts_config'])
                sf = EvaluationSelfPlay(mcts, config['self_play_number'],
                                        config['self_play_batch_size'])
                print('self-play:')
                self_play_file = self.get_cache_self_play_path()
                pre_sf_config = None
                for step, finished in sf.generator():
                    print('finished: {}/{} step: {}'
                          .format(finished, config['self_play_number'], step))
                    if step % config['self_play_cache_step'] == 0:
                        sf_config = sf.get_config()
                        json_dump_tuple(sf_config, self_play_file)
                        if pre_sf_config is not None:
                            EvaluationSelfPlay.from_config(pre_sf_config)
                        pre_sf_config = sf_config
                        if self.state == 0:
                            self.state = 1
                            self.cache()
                raw_samples = sf.get_raw_samples()
                json_dump_tuple(tuples_to_json(raw_samples), self.get_cache_self_play_sample_path())
                self.state = 2
                clear_temp_weight_files()
                self.cache()
                print('')

            elif self.state == 1:
                self_play_file = self.get_cache_self_play_path()
                sf_config = json_load_tuple(self_play_file)
                sf = EvaluationSelfPlay.from_config(sf_config, remove_weights=False)
                print('self-play:')
                generator = sf.generator()
                try:
                    step, finished = next(generator)
                except:
                    pass
                pre_sf_config = None
                for step, finished in sf.generator():
                    print('finished: {}/{} step: {}'
                          .format(finished, config['self_play_number'], step))
                    if step % config['self_play_cache_step'] == 0:
                        sf_config = sf.get_config()
                        json_dump_tuple(sf_config, self_play_file)
                        if pre_sf_config is not None:
                            EvaluationSelfPlay.from_config(pre_sf_config)
                        pre_sf_config = sf_config
                raw_samples = sf.get_raw_samples()
                json_dump_tuple(tuples_to_json(raw_samples), self.get_cache_self_play_sample_path())
                self.state = 2
                clear_temp_weight_files()
                self.cache()
                print('')

            elif self.state == 2:
                trainer = EvaluationTrainer(self.current_network.network,
                                            self.current_network.network.name,
                                            self.get_cache_self_play_sample_path(),
                                            **config['train'])
                self.state = 3
                self.cache()
                callbacks = [LearningRateScheduler(scheduler)]
                print('train:')
                trainer.train(True, 1.0, callbacks=callbacks)
                self.state = 4
                clear_temp_weight_files()
                self.cache()
                print('')

            elif self.state == 3:
                trainer = EvaluationTrainer.from_cache(self.current_network.network.name)
                callbacks = [LearningRateScheduler(scheduler)]
                print('train:')
                trainer.train(True, 1.0, callbacks=callbacks)
                self.state = 4
                clear_temp_weight_files()
                self.cache()
                print('')

            elif self.state == 4:
                evaluate_number = config['evaluate_number']
                evaluator = Evaluator(EvaluationMCTS(self.best_network, **config['evaluate_mcts_config']),
                                      EvaluationMCTS(self.current_network, **config['evaluate_mcts_config']),
                                      evaluate_number, config['evaluate_batch_size'])
                print('evaluation:')
                evaluate_file = self.get_cache_evaluate_path()
                pre_eval_config = None
                number = 2 * evaluate_number
                for step, finished, win, loss, draw in evaluator.generator():
                    print('finished: {}/{} step: {} win: {} loss: {} draw: {}'
                          .format(finished, number, step, win, loss, draw))
                    if win / float(number) >= config['evaluate_win_ratio']:
                        break
                    if (loss + draw) / float(number) > 1 - config['evaluate_win_ratio']:
                        break
                    if step % config['evaluate_cache_step'] == 0:
                        eval_config = evaluator.get_config()
                        json_dump_tuple(eval_config, evaluate_file)
                        if pre_eval_config is not None:
                            Evaluator.from_config(pre_eval_config)
                        pre_eval_config = eval_config
                        if self.state == 4:
                            self.state = 5
                            self.cache()
                print('\n')
                if win / float(number) >= config['evaluate_win_ratio']:
                    self.best_network = self.copy_network(self.current_network)
                    self.state = 0
                else:
                    self.state = 2
                clear_temp_weight_files()
                self.cache()

            else:
                evaluate_file = self.get_cache_evaluate_path()
                eval_config = json_load_tuple(evaluate_file)
                evaluator = Evaluator.from_config(eval_config, remove_weights=False)
                evaluate_number = config['evaluate_number']
                print('evaluation:')
                evaluate_file = self.get_cache_evaluate_path()
                pre_eval_config = None
                number = 2 * evaluate_number
                for step, finished, win, loss, draw in evaluator.generator():
                    print('finished: {}/{} step: {} win: {} loss: {} draw: {}'
                          .format(finished, number, step, win, loss, draw))
                    if win / float(number) >= config['evaluate_win_ratio']:
                        break
                    if (loss + draw) / float(number) > 1 - config['evaluate_win_ratio']:
                        break
                    if step % config['evaluate_cache_step'] == 0:
                        eval_config = evaluator.get_config()
                        json_dump_tuple(eval_config, evaluate_file)
                        if pre_eval_config is not None:
                            Evaluator.from_config(pre_eval_config)
                        pre_eval_config = eval_config
                print('\n')
                if win / float(number) >= config['evaluate_win_ratio']:
                    self.best_network = self.copy_network(self.current_network)
                    self.state = 0
                else:
                    self.state = 2
                clear_temp_weight_files()
                self.cache()

    def get_cache_folder(self):
        return get_cache_folder('rl_mixture')
