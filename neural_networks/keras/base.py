__all__ = ['PolicyBase', 'ValueBase', 'MixtureBase']
import os
import numpy as np
import keras.backend as K
import keras.engine as KE
from ...common import *
from ...board import Board
from ...utils.board_utils import action_functions
from ...utils.board_utils import tensor_functions
from ...utils.board_utils import recover_action_functions
from ...utils.board_utils import recover_tensor_functions
from ...utils.json_utils import json_load_tuple, json_dump_tuple
from .weights import get_weight_file


rng = np.random
data_format = K.image_data_format()


class Base(object):
    def __init__(self, network=None, batch_size=128, **kwargs):
        if network is not None and not isinstance(network, KE.Model):
            raise Exception('`network` must be an instance of Keras Model, '
                            'but got {}'.format(network))
        if network is not None:
            self.network = network
        else:
            self.network = self.initialize_network(**kwargs)
        self.batch_size = batch_size
        self.temp_functions = {}

    def initialize_network(self, **kwargs):
        raise NotImplementedError('`initialize_network` has not been implemented')

    def compile(self, *args, **kwargs):
        self.network.compile(*args, **kwargs)

    def fit(self, *args, **kwargs):
        self.network.fit(*args, **kwargs)

    def get_tensors(self, boards, transform=False):
        boards = tolist(boards)
        if not transform:
            tensors = [board.tensor for board in boards]
        else:
            tensors = []
            for board in boards:
                func = tensor_functions[np.random.randint(8)]
                tensors.append(func(board.tensor))
                self.temp_functions[board] = func
        shape = tensors[0].shape
        tensors = np.reshape(np.concatenate(tensors), (len(boards), )+shape)
        if data_format == 'channels_first':
            dim = len(tensors.shape)
            axes = list(range(dim))
            axes[1], axes[-1] = axes[-1], axes[1]
            return np.transpose(tensors, axes)
        elif data_format == 'channels_last':
            return tensors
        else:
            raise Exception('Unknown data format: {}'.format(data_format))

    def predict(self, boards, transform=False):
        boards = tolist(boards)
        tensors = self.get_tensors(boards, transform=transform)
        predicted_tensors = self.network.predict(tensors, batch_size=self.batch_size)
        if not transform:
            return predicted_tensors
        if isinstance(predicted_tensors, list):
            distributions, values = predicted_tensors
        elif predicted_tensors.shape[1] == 1:
            self.temp_functions.clear()
            return predicted_tensors
        else:
            distributions = predicted_tensors
            values = None
        unflatten_shape = (BOARD_SIZE, BOARD_SIZE)
        flatten_shape = (BOARD_SIZE**2, )
        for index, board in enumerate(boards):
            distribution = distributions[index, ...]
            func = self.temp_functions.pop(board)
            recover_func = recover_tensor_functions[func]
            distribution = recover_func(np.reshape(distribution, unflatten_shape))
            distributions[index, ...] = np.reshape(distribution, flatten_shape)
        if values is None:
            return distributions
        else:
            return distributions, values

    def save_weights(self, filepath):
        self.network.save_weights(filepath)

    def load_weights(self, filepath):
        if not os.path.exists(filepath):
            raise Exception('not found weight file')
        self.network.load_weights(filepath, by_name=True)

    def get_config(self, with_weights=False, pattern=None, index=None):
        network_config = self.network.get_config()
        config = {'batch_size': self.batch_size, 'network_config': network_config}
        if with_weights:
            config['weights'] = {'pattern': pattern, 'index': index}
        return config

    @classmethod
    def from_config(cls, config):
        network_config = config.pop('network_config')
        network = KE.Model.from_config(network_config)
        weights = config.pop('weights', None)
        object = cls(network=network, **config)
        if weights is not None:
            pattern = weights['pattern']
            network_name = network.name
            index = weights['index']
        object.load_weights(get_weight_file(pattern, network_name, index))
        return object

    def save_config(self, file_path, with_weights=False, pattern=None, index=None):
        config = self.get_config(with_weights, pattern, index)
        json_dump_tuple(config, file_path)

    @classmethod
    def load_config(cls, file_path):
        config = json_load_tuple(file_path)
        return cls.from_config(config)


class PolicyBase(Base):
    def sample(self, boards, distributions):
        boards = tolist(boards)
        actions = []
        for index, board in enumerate(boards):
            legal_actions = board.get_legal_actions()
            probs = [distributions[index, flatten(act)] for act in legal_actions]
            s = sum(probs)
            if s == 0.0:
                actions.append(self.zero_sum_exception(board, legal_actions, probs))
            else:
                probs = [p / s for p in probs]
                actions.append(legal_actions[multinomial_sampling(probs)])
        return tosingleton(actions)

    def predict_actions(self, boards, sample=True, transform=False):
        if sample:
            return self.sample(boards, self.predict(boards, transform=transform))
        distributions = self.predict(boards, transform=transform)
        distributions = [distributions[index, ...]
                         for index in range(len(tolist(boards)))]
        return tosingleton(distributions)

    def zero_sum_exception(self, board, legal_actions, probs):
        print('warning: zero sum of legal actions, '
              'select an action randomly')
        return legal_actions[rng.randint(len(legal_actions))]


class ValueBase(Base):
    def value_tolist(self, boards, values):
        return tosingleton([float(values[i]) for i in range(len(tolist(boards)))])

    def predict_values(self, boards, transform=False):
        return self.value_tolist(boards, self.predict(boards, transform=transform))


class MixtureBase(PolicyBase, ValueBase):
    def predict_actions(self, boards, sample=True, transform=False):
        distributions, _ = self.predict(boards, transform=transform)
        if sample:
            return self.sample(boards, distributions)
        distributions = [distributions[index, ...] for index in range(len(tolist(boards)))]
        return tosingleton(distributions)

    def predict_values(self, boards, transform=False):
        _, values = self.predict(boards, transform=transform)
        return self.value_tolist(boards, values)

    def predict_pairs(self, boards, sample=True, transform=False):
        distributions, values = self.predict(boards, transform=transform)
        if sample:
            return self.sample(boards, distributions), \
                   self.value_tolist(boards, values)
        distributions = [distributions[index, ...]
                         for index in range(len(tolist(boards)))]
        return tosingleton(distributions), self.value_tolist(boards, values)
