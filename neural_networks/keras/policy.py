__all__ = ['ResNetPolicy', 'UnitizedResNetPolicy']
import keras.backend as K
import keras.layers as KL
import keras.engine as KE
import keras.initializers as KI
from ...global_constants import *
from ...board import Board
from .base import PolicyBase
from .layers import Unitization
from .models import get_resnet, get_unitized_resnet

AXIS = 1 if K.image_data_format() == 'channels_first' else -1

class ResNetPolicy(PolicyBase):
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
            name='last_convolution')(resnet_outputs)
        tensor = KL.BatchNormalization(axis=AXIS, name='last_batch_norm')(tensor)
        tensor = KL.Activation(activation='relu', name='last_relu')(tensor)
        tensor = KL.Flatten(name='flatten')(tensor)
        outputs = KL.Dense(BOARD_SIZE**2, activation='softmax', name='distribution')(tensor)
        model = KE.Model(resnet_inputs, outputs,
                         name='resnet_{}_policy'.format(6*stack_nb+2))
        # model.summary()
        return model


class UnitizedResNetPolicy(PolicyBase):
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
            name='last_convolution')(resnet_outputs)
        tensor = Unitization(axis=AXIS, name='last_unitization')(tensor)
        tensor = KL.Activation(activation='relu', name='last_relu')(tensor)
        tensor = KL.Flatten(name='flatten')(tensor)
        outputs = KL.Dense(BOARD_SIZE**2, activation='softmax', name='distribution')(tensor)
        model = KE.Model(resnet_inputs, outputs,
                         name='unitized_resnet_{}_policy'.format(6*stack_nb+2))
        # model.summary()
        return model
