__all__ = ['ResNetValue']
import keras.layers as KL
import keras.engine as KE
from ...global_constants import *
from ...board import Board
from .base import ValueBase
from .models import get_resnet

class ResNetValue(ValueBase):
    def initialize_network(self, stack_nb, board_cls=Board, weight_decay=0.0005):
        if not isinstance(board_cls, type) or not issubclass(board_cls, Board):
            raise Exception('`board_cls` must be a class/subclass of `Board`')
        board = Board(toTensor=True)
        resnet_inputs, resnet_outputs = get_resnet(board.tensor.shape,
                                                   stack_nb, weight_decay)
        tensor = KL.GlobalAveragePooling2D(name='global_pooling')(resnet_outputs)
        outputs = KL.Dense(1, activation='tanh', name='value')(tensor)
        return KE.Model(resnet_inputs, outputs)
