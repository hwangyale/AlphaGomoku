__all__ = ['ResNetMixture']
import keras.backend as K
import keras.layers as KL
import keras.engine as KE
import keras.initializers as KI
from ...global_constants import *
from ...board import Board
from .base import MixtureBase
from .layers import Unitization
from .models import get_resnet

AXIS = 1 if K.image_data_format() == 'channels_first' else -1

class ResNetMixture(MixtureBase):
    def initialize_network(self, stack_nb, board_cls=Board, weight_decay=0.0005):
        if not isinstance(board_cls, type) or not issubclass(board_cls, Board):
            raise Exception('`board_cls` must be a class/subclass of `Board`')
        board = Board(toTensor=True)
        resnet_inputs, resnet_outputs = get_resnet(board.tensor.shape,
                                                   stack_nb, weight_decay)
        tensor = KL.Conv2D(
            filters=2,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            name='last_policy_convolution')(resnet_outputs)
        tensor = KL.BatchNormalization(axis=AXIS, name='last_policy_batch_norm')(tensor)
        tensor = KL.Activation(activation='relu', name='last_policy_relu')(tensor)
        tensor = KL.Flatten(name='policy_flatten')(tensor)
        distributions = KL.Dense(BOARD_SIZE**2, activation='softmax', name='distribution')(tensor)

        tensor = KL.Conv2D(
            filters=1,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            name='last_value_convolution')(resnet_outputs)
        tensor = KL.BatchNormalization(axis=AXIS, name='last_value_batch_norm')(tensor)
        tensor = KL.Activation(activation='relu', name='last_value_relu')(tensor)
        tensor = KL.Flatten(name='value_flatten')(tensor)
        tensor = KL.Dense(256, activation='relu', name='full_connected_layer')(tensor)
        values = KL.Dense(1, activation='tanh', name='value')(tensor)
        return KE.Model(resnet_inputs, [distributions, values],
                        name='resnet_{}_mixture'.format(6*stack_nb+2))


class UnitizedResNetMixture(MixtureBase):
    def initialize_network(self, stack_nb, board_cls=Board, weight_decay=0.0005):
        if not isinstance(board_cls, type) or not issubclass(board_cls, Board):
            raise Exception('`board_cls` must be a class/subclass of `Board`')
        board = Board(toTensor=True)
        resnet_inputs, resnet_outputs = get_unitized_resnet(board.tensor.shape,
                                                            stack_nb, weight_decay)
        tensor = KL.Conv2D(
            filters=2,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            name='last_policy_convolution')(resnet_outputs)
        tensor = Unitization(axis=AXIS, name='last_policy_unitization')(tensor)
        tensor = KL.Activation(activation='relu', name='last_policy_relu')(tensor)
        tensor = KL.Flatten(name='policy_flatten')(tensor)
        distributions = KL.Dense(BOARD_SIZE**2, activation='softmax', name='distribution')(tensor)

        tensor = KL.Conv2D(
            filters=1,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            name='last_value_convolution')(resnet_outputs)
        tensor = Unitization(axis=AXIS, name='last_value_unitization')(tensor)
        tensor = KL.Activation(activation='relu', name='last_value_relu')(tensor)
        tensor = KL.Flatten(name='value_flatten')(tensor)
        tensor = KL.Dense(256, activation='relu', name='full_connected_layer')(tensor)
        values = KL.Dense(1, activation='tanh', name='value')(tensor)
        return KE.Model(resnet_inputs, [distributions, values],
                        name='unitized_resnet_{}_mixture'.format(6*stack_nb+2))
