from ..global_constants import *
from ..common import *
from ..player import PlayerBase
from ..neural_networks import get_network


class NetworkAI(PlayerBase):
    def __init__(self, network_type='resnet', *args, **kwargs):
        self.network = get_network('policy', network_type, *args, **kwargs)

    def load_weights(self, filepath):
        self.network.load_weights(filepath)

    def get_action(self, board):
        return self.network.predict_actions(board)
