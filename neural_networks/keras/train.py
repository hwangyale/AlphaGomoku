import os
import random
import numpy as np
import keras.optimizers as KO
import keras.callbacks as KC
import keras.engine as KE
from ...global_constants import *
from ...common import *
from ...utils.json_utils import json_dump_tuple, json_load_tuple
from ...temp import get_cache_folder, remove_folder
from .weights import get_config_file, get_weight_file
from ...utils.board_utils import action_functions, tensor_functions
from ...process_data import process_history

def optimizer_wrapper(optimizer):
    if not isinstance(optimizer, KO.Optimizer):
        raise Exception('`optimizer` must be an instance of `Optimizer`')

    def load_weights(self, file_path):
        npz_file = np.load(file_path)
        self.set_weights([npz_file['arr_{}'.format(i)]
                          for i in range(len(npz_file))])

    def save_weights(self, file_path)
        weights = self.get_weights()
        np.savez(file_path, *weights)

    optimizer.load_weights = lambda file_path : load_weights(optimizer, file_path)
    optimizer.save_weights = lambda file_path : save_weights(optimizer, file_path)
    return optimizer

class CacheCallback(KC.Callback):
    def __init__(self, begin_save, epoch_save, folder):
        self.begin_save = begin_save
        self.epoch_save = epoch_save
        self.folder = folder

    def on_train_begin(self, logs=None):
        self.begin_save()

    def on_epoch_end(self, logs=None):
        self.epoch_save()

    def on_train_end(self, logs=None):
        remove_folder(self.folder)

class TrainerBase(object):
    def __init__(self, network, network_name, data_file, **kwargs):
        self.network = network
        self.trainer_params = {
            'network_name': network_name, 'data_file': data_file
        }
        self.optimizer_params = {
            'class_name': kwargs.get('optimizer', KO.SGD.__name__),
            'config': kwargs.get('optimizer_config',
                                 {'lr': 0.01, 'momentum': 0.9, 'nesterov': True})
        }
        self.compile_params = {
            'loss': self.get_loss(),
            'metrics': kwargs.get('metrics', ['acc']),
            'loss_weights': self.get_loss_weights()
        }
        self.train_params = {
            'batch_size': kwargs.get('batch_size', 128),
            'epochs': kwargs.get('epochs', 16),
            'verbose': kwargs.get('verbose', 1),
            'initial_epoch': kwargs.get('initial_epoch', 0)
        }
        self.compiled = False

    def compile(self):
        self.optimizer = optimizer_wrapper(KO.get(self.optimizer_params))
        self.network.compile(optimizer=self.optimizer, **self.optimizer_params)
        self.network._make_train_function()
        self.compiled = True

    def train(self, cache=True):
        if not self.compiled:
            self.compile()
        generator, steps_per_epoch = self.get_generator(cache=cache,
                                     batch_size=self.train_params['batch_size'])
        validation_data = self.get_validation_data()
        kwargs = {'generator': generator, 'steps_per_epoch': steps_per_epoch,
                  'validation_data': validation_data}
        kwargs.update(self.train_params)
        if cache:
            json_dump_tuple(self.trainer_params, self.get_trainer_path())
            json_dump_tuple(self.optimizer_params, self.get_optimizer_path()[0])
            json_dump_tuple(self.compile_params, self.get_compile_path())
            json_dump_tuple(self.network.get_config(), self.get_network_path()[0])
            folder = self.get_cache_folder()
            kwargs['callbacks'] = [CacheCallback(self.get_begin_save(),
                                   self.get_epoch_save(), folder)]
        return self.network.fit_generator(**kwargs).history

    def get_begin_save(self):
        def save():
            self.train_config_file = self.get_train_path()
            self.optimizer_weight_file = self.get_optimizer_path()[1]
            self.network_weight_file = self.get_network_path()[1]
            json_dump_tuple(self.train_params, self.train_config_file)
            self.optimizer.save_weights(self.optimizer_weight_file)
            self.network.save_weights(self.network_weight_file)
        return save

    def get_epoch_save(self):
        def save():
            self.train_params['initial_epoch'] += 1
            json_dump_tuple(self.train_params, self.train_config_file)
            self.optimizer.save_weights(self.optimizer_weight_file)
            self.network.save_weights(self.network_weight_file)
        return save

    @classmethod
    def from_cache(cls, network_name):
        trainer = cls(None, network_name, '')
        network_config_file, network_weight_file = trainer.get_network_path()
        network = KE.from_config(json_load_tuple(network_config_file))
        network.load_weights(network_weight_file)
        trainer.trainer_params.update(json_load_tuple(trainer.get_trainer_path()))
        trainer.compile_params.update(json_load_tuple(trainer.get_compile_path()))
        optimizer_config_file, optimizer_weight_file = trainer.get_optimizer_path()
        trainer.optimizer_params.update(json_load_tuple(optimizer_config_file))
        trainer.compile()
        trainer.optimizer.load_weights(optimizer_weight_file)
        trainer.train_params.update(json_load_tuple(trainer.get_train_path()))
        return trainer

    def get_loss(self):
        raise Exception('Not implemented')

    def get_loss_weights(self):
        raise Exception('Not implemented')

    def get_generator(self, cache, batch_size, split):
        raise Exception('Not implemented')

    def get_validation_data(self):
        return None

    def get_cache_folder(self):
        raise Exception('Not implemented')

    def load_cache_tuples(self):
        folder = self.get_cache_folder()
        file_name = 'tuple_container.json'
        tuple_container_file = os.path.join(folder, file_name)
        self.tuple_container = json_load_tuple(tuple_container_file)

    def load_tuples(self, cache, split=0.9):
        if not hasattr(self, 'tuple_container'):
            history_container = json_load_tuple(self.trainer_params['data_file'])
            tuples = process_history(history_container, False)
            np.random.shuffle(tuples)
            split_index = int(split * len(tuples))
            train_tuple_container = tuples[:split_index]
            test_tuple_container = tuples[split_index:]
            self.tuple_container = [train_tuple_container, test_tuple_container]
            if cache:
                folder = self.get_cache_folder()
                file_name = 'tuple_container.json'
                tuple_container_file = os.path.join(folder, file_name)
                json_dump_tuple(self.tuple_container, tuple_container_file)

    def get_trainer_path(self):
        file_name = 'config.json'
        return os.path.join(self.get_cache_folder(), file_name)

    def get_compile_path(self):
        file_name = 'compile_config.json'
        return os.path.join(self.get_cache_folder(), file_name)

    def get_optimizer_path(self):
        config_file_name = 'optimizer_config.json'
        weight_file_name = 'optimizer_weights.npz'
        folder = self.get_cache_folder()
        return os.path.join(folder, config_file_name), \
               os.path.join(folder, weight_file_name)

    def get_network_path(self):
        config_file_name = 'network_config.json'
        weight_file_name = 'network_weights.npz'
        folder = self.get_cache_folder()
        return os.path.join(folder, config_file_name), \
               os.path.join(folder, weight_file_name)

    def get_train_path(self):
        config_file_name = 'train_config.json'
        return os.path.join(self.get_cache_folder(), config_file_name)

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


class PrePolicyTrainer(TrainerBase):
    def get_loss(self):
        return 'categorical_crossentropy'

    def get_loss_weights(self):
        return None

    def get_generator(self, cache, batch_size, split):
        self.load_tuples(cache, split)
        train_tuple_container = self.tuple_container[0]
        pairs = [(self.get_tensor_from_history(history), action)
                 for history, action in train_tuple_container]
        def generator():
            number = len(pairs)
            batches = KE.training._make_batches(number, batch_size)
            function_pairs = list(zip(tensor_functions, action_functions))
            while True:
                indice = list(range(number))
                np.random.shuffle(indice)
                for batch_begin, batch_end in batches:
                    Xs = []
                    Ys = []
                    for index in indice[batch_begin:batch_end]:
                        tensor, action = pairs[index]
                        tensor_func, action_func = random.choice(function_pairs)
                        Xs.append(np.expand_dims(tensor_func(tensor), 0))
                        y = np.zero((1, BOARD_SIZE**2))
                        y[flatten(action_func(action))] = 1.0
                        Ys.append(y)
                    yield np.concatenate(Xs, axis=0), np.concatenate(Ys, axis=0)
        return generator(), (len(pairs) + batch_size - 1) // batch_size

    def get_validation_data(self):
        test_tuple_container = self.tuple_container[1]
        if len(test_tuple_container) == 0:
            return None
        pairs = [(self.get_tensor_from_history(history), action)
                 for history, action in test_tuple_container]
        Xs = []
        Ys = []
        for tensor_func, action_func in zip(tensor_functions, action_functions):
            for tensor, action in pairs:
                Xs.append(np.expand_dims(tensor_func(tensor), 0))
                y = np.zeros((1, BOARD_SIZE**2))
                y[flatten(action_func(action))] = 1.0
                Ys.append(y)
        return np.concatenate(Xs, axis=0), np.concatenate(Ys, axis=0)

    def get_cache_folder(self):
        get_cache_folder('pre_policy')
        return get_cache_folder('pre_policy\\{}'
                                .format(self.trainer_params['network_name']))
