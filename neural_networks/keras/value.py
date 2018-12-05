__all__ = ['ResNetValue']
import keras.backend as K
import keras.layers as KL
import keras.engine as KE
from ...global_constants import *
from ...board import Board
from .base import ValueBase
from .layers import Unitization
from .models import get_resnet, get_unitized_resnet

AXIS = 1 if K.image_data_format() == 'channels_first' else -1

class ResNetValue(ValueBase):
    def initialize_network(self, stack_nb, board_cls=Board, weight_decay=0.0005):
        if not isinstance(board_cls, type) or not issubclass(board_cls, Board):
            raise Exception('`board_cls` must be a class/subclass of `Board`')
        board = Board(toTensor=True)
        resnet_inputs, resnet_outputs = get_resnet(board.tensor.shape,
                                                   stack_nb, weight_decay)
        # tensor = KL.GlobalAveragePooling2D(name='global_pooling')(resnet_outputs)
        tensor = KL.Conv2D(
            filters=1,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            name='last_convolution')(resnet_outputs)
        tensor = KL.BatchNormalization(axis=AXIS, name='last_batch_norm')(tensor)
        tensor = KL.Activation(activation='relu', name='last_relu')(tensor)
        tensor = KL.Flatten(name='flatten')(tensor)
        tensor = KL.Dense(256, activation='relu', name='full_connected_layer')(tensor)
        outputs = KL.Dense(1, activation='tanh', name='value')(tensor)
        return KE.Model(resnet_inputs, outputs,
                        name='resnet_{}_value'.format(6*stack_nb+2))


class UnitizedResNetValue(ValueBase):
    def initialize_network(self, stack_nb, board_cls=Board, weight_decay=0.0005):
        if not isinstance(board_cls, type) or not issubclass(board_cls, Board):
            raise Exception('`board_cls` must be a class/subclass of `Board`')
        board = Board(toTensor=True)
        resnet_inputs, resnet_outputs = get_unitized_resnet(board.tensor.shape,
                                                            stack_nb, weight_decay)
        # tensor = KL.GlobalAveragePooling2D(name='global_pooling')(resnet_outputs)
        tensor = KL.Conv2D(
            filters=1,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            name='last_convolution')(resnet_outputs)
        tensor = Unitization(axis=AXIS, name='last_unitization')(tensor)
        tensor = KL.Activation(activation='relu', name='last_relu')(tensor)
        tensor = KL.Flatten(name='flatten')(tensor)
        tensor = KL.Dense(256, activation='relu', name='full_connected_layer')(tensor)
        outputs = KL.Dense(1, activation='tanh', name='value')(tensor)
        return KE.Model(resnet_inputs, outputs,
                        name='unitized_resnet_{}_value'.format(6*stack_nb+2))
