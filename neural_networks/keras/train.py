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
from ...process_data import tuples_to_json, json_to_tuples
from ...utils.keras_utils import optimizer_wrapper, CacheCallback


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
            'metrics': self.get_metrics(),
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
        self.network.compile(optimizer=self.optimizer, **self.compile_params)
        self.network._make_train_function()
        self.compiled = True

    def train(self, cache=True, split=0.9, **kwargs):
        if not self.compiled:
            self.compile()
        generator, steps_per_epoch = self.get_generator(
            cache=cache, batch_size=self.train_params['batch_size'], split=split
        )
        validation_data = self.get_validation_data()
        kwargs.update({'generator': generator, 'steps_per_epoch': steps_per_epoch,
                       'validation_data': validation_data})
        kwargs.update(self.train_params)
        if cache:
            json_dump_tuple(self.trainer_params, self.get_trainer_path())
            json_dump_tuple(self.optimizer_params, self.get_optimizer_path()[0])
            json_dump_tuple(self.compile_params, self.get_compile_path())
            json_dump_tuple(self.network.get_config(), self.get_network_path()[0])
            folder = self.get_cache_folder()
            if 'callbacks' not in kwargs:
                kwargs['callbacks'] = []
            kwargs['callbacks'] += [CacheCallback(self.get_begin_save(),
                                    self.get_epoch_save(), folder)]
        kwargs.pop('batch_size')
        return self.network.fit_generator(**kwargs).history

    def get_begin_save(self):
        def save():
            self.train_config_file = self.get_train_path()
            json_dump_tuple(self.train_params, self.train_config_file)
            optimizer_weight_file = self.get_optimizer_path()[1]
            self.optimizer.save_weights(optimizer_weight_file)
            self.cache_optimizer_weight_file = optimizer_weight_file
            network_weight_file = self.get_network_path()[1]
            self.network.save_weights(network_weight_file)
            self.cache_network_weight_file = network_weight_file
        return save

    def get_epoch_save(self):
        def save():
            self.train_params['initial_epoch'] += 1
            optimizer_weight_file = self.get_optimizer_path()[1]
            self.optimizer.save_weights(optimizer_weight_file)
            network_weight_file = self.get_network_path()[1]
            self.network.save_weights(network_weight_file)
            json_dump_tuple(self.train_params, self.train_config_file)
            os.remove(self.cache_optimizer_weight_file)
            self.cache_optimizer_weight_file = optimizer_weight_file
            os.remove(self.cache_network_weight_file)
            self.cache_network_weight_file = network_weight_file
        return save

    @classmethod
    def from_cache(cls, network_name):
        trainer = cls(None, network_name, '')
        trainer.train_params.update(json_load_tuple(trainer.get_train_path()))
        network_config_file, network_weight_file = trainer.get_network_path()
        network = KE.Model.from_config(json_load_tuple(network_config_file))
        network.load_weights(network_weight_file)
        trainer.network = network
        trainer.trainer_params.update(json_load_tuple(trainer.get_trainer_path()))
        trainer.compile_params.update(json_load_tuple(trainer.get_compile_path()))
        optimizer_config_file, optimizer_weight_file = trainer.get_optimizer_path()
        trainer.optimizer_params.update(json_load_tuple(optimizer_config_file))
        trainer.compile()
        trainer.optimizer.load_weights(optimizer_weight_file)
        return trainer

    def get_loss(self):
        raise Exception('Not implemented')

    def get_metrics(self):
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
        self.tuple_container = [json_to_tuples(json_object)
                                for json_object in json_load_tuple(tuple_container_file)]

    def load_tuples(self, cache, split=0.9):
        if not hasattr(self, 'tuple_container'):
            tuples = json_to_tuples(json_load_tuple(self.trainer_params['data_file']))
            if split < 1.0:
                np.random.shuffle(tuples)
                split_index = int(split * len(tuples))
                train_tuple_container = tuples[:split_index]
                test_tuple_container = tuples[split_index:]
                self.tuple_container = [train_tuple_container, test_tuple_container]
            else:
                self.tuple_container = [tuples, []]
            if cache:
                folder = self.get_cache_folder()
                file_name = 'tuple_container.json'
                tuple_container_file = os.path.join(folder, file_name)
                json_dump_tuple(
                    [tuples_to_json(tuples) for tuples in self.tuple_container],
                    tuple_container_file
                )

    def get_trainer_path(self):
        file_name = 'config.json'
        return os.path.join(self.get_cache_folder(), file_name)

    def get_compile_path(self):
        file_name = 'compile_config.json'
        return os.path.join(self.get_cache_folder(), file_name)

    def get_optimizer_path(self):
        config_file_name = 'optimizer_config.json'
        epoch = self.train_params['initial_epoch']
        weight_file_name = 'optimizer_weights_{}.npz'.format(epoch)
        folder = self.get_cache_folder()
        return os.path.join(folder, config_file_name), \
               os.path.join(folder, weight_file_name)

    def get_network_path(self):
        config_file_name = 'network_config.json'
        epoch = self.train_params['initial_epoch']
        weight_file_name = 'network_weights_{}.npz'.format(epoch)
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

    def get_metrics(self):
        return ['acc']

    def get_loss_weights(self):
        return None

    def get_generator(self, cache, batch_size, split):
        self.load_tuples(cache, split)
        train_tuple_container = self.tuple_container[0]
        pairs = [(self.get_tensor_from_history(history), action)
                 for history, action, _ in train_tuple_container]
        def generator():
            number = len(pairs)
            batches = KE.training._make_batches(number, batch_size)
            function_pairs = list(zip(tensor_functions, action_functions))
            while True:
                indice = list(range(number))
                np.random.shuffle(indice)
                for batch_begin, batch_end in batches:
                    Xs = []
                    Ys = np.zeros((batch_end-batch_begin, BOARD_SIZE**2), dtype=FLOATX)
                    for idx, index in enumerate(indice[batch_begin:batch_end]):
                        tensor, action = pairs[index]
                        tensor_func, action_func = function_pairs[np.random.randint(8)]
                        Xs.append(np.expand_dims(tensor_func(tensor), 0))
                        Ys[idx, flatten(action_func(action))] = 1.0
                    yield np.concatenate(Xs, axis=0), Ys
        return generator(), (len(pairs) + batch_size - 1) // batch_size

    def get_validation_data(self):
        test_tuple_container = self.tuple_container[1]
        if len(test_tuple_container) == 0:
            return None
        pairs = [(self.get_tensor_from_history(history), action)
                 for history, action, _ in test_tuple_container]
        Xs = []
        Ys = []
        for tensor_func, action_func in zip(tensor_functions, action_functions):
            for tensor, action in pairs:
                Xs.append(np.expand_dims(tensor_func(tensor), 0))
                y = np.zeros((1, BOARD_SIZE**2), dtype=FLOATX)
                y[0, flatten(action_func(action))] = 1.0
                Ys.append(y)
        return np.concatenate(Xs, axis=0), np.concatenate(Ys, axis=0)

    def get_cache_folder(self):
        get_cache_folder('pre_policy')
        return get_cache_folder('pre_policy\\{}'
                                .format(self.trainer_params['network_name']))


class PreValueTrainer(TrainerBase):
    def get_loss(self):
        return 'mean_squared_error'

    def get_metrics(self):
        return None

    def get_loss_weights(self):
        return None

    def get_generator(self, cache, batch_size, split):
        self.load_tuples(cache, split)
        train_tuple_container = self.tuple_container[0]
        pairs = [(self.get_tensor_from_history(history), value)
                 for history, _, value in train_tuple_container]
        def generator():
            number = len(pairs)
            batches = KE.training._make_batches(number, batch_size)
            while True:
                indice = list(range(number))
                np.random.shuffle(indice)
                for batch_begin, batch_end in batches:
                    Xs = []
                    Ys = np.zeros((batch_end-batch_begin, 1), dtype=FLOATX)
                    for idx, index in enumerate(indice[batch_begin:batch_end]):
                        tensor, value = pairs[index]
                        tensor_func = tensor_functions[np.random.randint(8)]
                        Xs.append(np.expand_dims(tensor_func(tensor), 0))
                        Ys[idx, 0] = value
                    yield np.concatenate(Xs, axis=0), Ys
        return generator(), (len(pairs) + batch_size - 1) // batch_size

    def get_validation_data(self):
        test_tuple_container = self.tuple_container[1]
        if len(test_tuple_container) == 0:
            return None
        pairs = [(self.get_tensor_from_history(history), value)
                 for history, _, value in test_tuple_container]
        Xs = []
        Ys = np.zeros((len(pairs)*len(tensor_functions), 1), dtype=FLOATX)
        index = 0
        for tensor_func in tensor_functions:
            for tensor, value in pairs:
                Xs.append(np.expand_dims(tensor_func(tensor), 0))
                Ys[index, 0] = value
                index += 1
        return np.concatenate(Xs, axis=0), Ys

    def get_cache_folder(self):
        get_cache_folder('pre_value')
        return get_cache_folder('pre_value\\{}'
                                .format(self.trainer_params['network_name']))


class PreMixtureTrainer(TrainerBase):
    def get_loss(self):
        return {'distribution': 'categorical_crossentropy',
                'value': 'mean_squared_error'}

    def get_metrics(self):
        return {'distribution': 'acc'}

    def get_loss_weights(self):
        return {'distribution': 1.0, 'value': 1.0}

    def get_generator(self, cache, batch_size, split):
        self.load_tuples(cache, split)
        train_tuple_container = self.tuple_container[0]
        tuples = [(self.get_tensor_from_history(history), action, value)
                  for history, action, value in train_tuple_container]
        def generator():
            number = len(tuples)
            batches = KE.training._make_batches(number, batch_size)
            function_pairs = list(zip(tensor_functions, action_functions))
            while True:
                indice = list(range(number))
                np.random.shuffle(indice)
                for batch_begin, batch_end in batches:
                    Xs = []
                    Ys = np.zeros((batch_end-batch_begin, BOARD_SIZE**2), dtype=FLOATX)
                    Zs = np.zeros((batch_end-batch_begin, 1), dtype=FLOATX)
                    for idx, index in enumerate(indice[batch_begin:batch_end]):
                        tensor, action, value = tuples[index]
                        tensor_func, action_func = function_pairs[np.random.randint(8)]
                        Xs.append(np.expand_dims(tensor_func(tensor), 0))
                        Ys[idx, flatten(action_func(action))] = 1.0
                        Zs[idx, 0] = value
                    yield np.concatenate(Xs, axis=0), [Ys, Zs]
        return generator(), (len(tuples) + batch_size - 1) // batch_size

    def get_validation_data(self):
        test_tuple_container = self.tuple_container[1]
        if len(test_tuple_container) == 0:
            return None
        tuples = [(self.get_tensor_from_history(history), action, value)
                  for history, action, value in test_tuple_container]
        Xs = []
        Ys = np.zeros((len(tuples)*len(tensor_functions), BOARD_SIZE**2), dtype=FLOATX)
        Zs = np.zeros((len(tuples)*len(tensor_functions), 1), dtype=FLOATX)
        index = 0
        for tensor_func, action_func in zip(tensor_functions, action_functions):
            for tensor, action, value in tuples:
                Xs.append(np.expand_dims(tensor_func(tensor), 0))
                Ys[index, flatten(action_func(action))] = 1.0
                Zs[index, 0] = value
                index += 1
        return np.concatenate(Xs, axis=0), [Ys, Zs]

    def get_cache_folder(self):
        get_cache_folder('pre_mixture')
        return get_cache_folder('pre_mixture\\{}'
                                .format(self.trainer_params['network_name']))
