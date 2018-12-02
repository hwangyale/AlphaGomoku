__all__ = ['ResNetMixture']
import keras.layers as KL
import keras.engine as KE
import keras.initializers as KI
from ...global_constants import *
from ...board import Board
from .base import MixtureBase
from .models import get_resnet

class ResNetMixture(MixtureBase):
    def initialize_network(self, stack_nb, board_cls=Board, weight_decay=0.0005):
        if not isinstance(board_cls, type) or not issubclass(board_cls, Board):
            raise Exception('`board_cls` must be a class/subclass of `Board`')
        board = Board(toTensor=True)
        resnet_inputs, resnet_outputs = get_resnet(board.tensor.shape,
                                                   stack_nb, weight_decay)
        tensor = KL.Conv2D(
            filters=1,
            kernel_size=(1, 1),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            activation='softmax',
            name='unflattened_distribution')(resnet_outputs)
        distributions = KL.Flatten(name='distribution')(tensor)

        tensor = KL.GlobalAveragePooling2D(name='global_pooling')(resnet_outputs)
        values = KL.Dense(1, activation='tanh', name='value')(tensor)
        return KE.Model(resnet_inputs, [distributions, values])
