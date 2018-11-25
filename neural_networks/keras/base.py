__all__ = ['NeuralNetworkBase']
import numpy as np
import keras.backend as K
import keras.engine as KE
from ...common import *
from ...board import Board


rng = np.random
data_format = K.image_data_format()


class Base(object):
    def __init__(self, network=None, batch_size=128, **kwargs):
        if not isinstance(network, KE.Model):
            raise Exception('`network` must be an instance of Keras Model, '
                            'but got {}'.format(network))
        if network is not None:
            self.network = network
        else:
            self.network = self.initialize_network(**kwargs)
        self.batch_size = batch_size

    def initialize_network(self, **kwargs):
        raise NotImplementedError('`initialize_network` has not been implemented')

        def train(self, **kwargs):
            raise NotImplementedError('`train` has not been implemented')

    def get_tensors(self, boards):
        boards = tolist(boards)
        tensors = [board.tensor for board in boards]
        shape = tensors[0].shape
        tensors = np.reshape(np.concatenate(tensors), (len(boards), )+shape)
        if data_format == 'channels_first':
            return tensors
        elif data_format == 'channels_last':
            dim = len(tensors.shape)
            axes = list(range(dim))
            axes[1], axes[-1] = axes[-1], axes[1]
            return np.transpose(tensors, axes)
        else:
            raise Exception('Unknown data format: {}'.format(data_format))

    def predict(self, boards):
        tensors = self.get_tensors(boards)
        return self.network.predict(tensors, batch_size=self.batch_size)

    def save_weights(self, filepath):
        self.network.save_weights(filepath)

    def load_weights(self, filepath):
        self.network.load_weights(filepath, by_name=True)


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

    def predict_actions(self, boards):
        return sample(boards, self.predict(boards))

    def zero_sum_exception(self, board, legal_actions, probs):
        print('warning: zero sum of legal actions, '
              'select an action randomly')
        return legal_actions[rng.randint(len(legal_actions))]


class ValueBase(Base):
    def predict_values(self, boards):
        boards = tolist(boards)
        values = self.predict(boards)
        return tosingleton([values[i] for i in range(len(boards))])
