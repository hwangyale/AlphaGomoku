__all__ = ['ResNetMixture']
import keras.layers as KL
import keras.engine as KE
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
        distributions = KL.Dense(BOARD_SIZE**2, activation='softmax',
                                 name='distribution')(resnet_outputs)
        values = KL.Dense(1, activation='tanh', name='value')(resnet_outputs)
        return KE.Model(resnet_inputs, [distributions, values])