import os
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..mcts import get as mcts_get
from ..neural_networks.keras import get as network_get
from ..utils.json_utils import json_dump_tuple, json_load_tuple


rng = np.random


class SelfPlay(object):
    def __init__(self, mcts, number=1000, batch_size=10, initialize=True):
        self.mcts = mcts
        self.number = number
        self.batch_size = batch_size
        if initialize:
            index = min(batch_size, number)
            self.index = index
            self.boards = set(Board() for _ in range(index))
            self.step = 0

    def run(self, cache_step=None, cache_folder=''):
        mcts = self.mcts
        number = self.number
        batch_size = self.batch_size
        index = self.index
        boards = self.boards
        if cache_step is not None:
            if isinstance(mcts, mcts_get('EvaluationMCTSBase')):
                config_path, weight_path = self.get_mixture_network_path(folder)
                json_dump_tuple(mcts.mixture_network.get_config(), config_path)
                mcts.mixture_network.save_weights(weight_path)
            else:
                policy_config_path, policy_weight_path = self.get_policy_network_path(folder)
                json_dump_tuple(mcts.policy_network.get_config(), policy_config_path)
                mcts.policy.save_weights(policy_weight_path)
                rollout_config_path, rollout_weight_path = self.get_rollout_network_path(folder)
                json_dump_tuple(mcts.rollout_network.get_config(), rollout_config_path)
                mcts.rollout.save_weights(rollout_weight_path)
            config_path = self.get_config_path(folder)
        while len(boards) > 0:
            temp_boards = list(boards)
            actions = tolist(mcts.mcts(temp_boards, verbose=0))
            for board, action in zip(temp_boards, actions):
                board.move(action)
                if board.is_over:
                    boards.remove(board)
                    if index < number:
                        boards.add(Board())
                        index += 1
            self.step += 1
            if cache_step is not None and self.step % cache_step == 0:
                json_dump_tuple(self.get_config(), config_path)

    @classmethod
    def get_mixture_network_path(cls, folder):
        config_path = os.path.join(folder, 'mixture_network_config.json')
        weight_path = os.path.join(folder, 'mixture_network_weights.npz')
        return config_path, weight_path

    @classmethod
    def get_policy_network_path(cls, folder):
        policy_config_path = os.path.join(folder, 'policy_network_config.json')
        policy_weight_path = os.path.join(folder, 'policy_network_weights.npz')
        return policy_config_path, policy_weight_path

    @classmethod
    def get_rollout_network_path(cls, folder):
        rollout_config_path = os.path.join(folder, 'rollout_network_config.json')
        rollout_weight_path = os.path.join(folder, 'rollout_network_weights.npz')
        return rollout_config_path, rollout_weight_path

    @classmethod
    def get_config_path(cls, folder):
        return os.path.join(folder, 'self_play.json')

    def get_config(self):
        config = [
            'mcts_cls_name': self.mcts.__class__.__name__,
            'mcts_config': self.mcts.get_config(),
            'number': self.number,
            'batch_size': self.batch_size,
            'index': self.index,
            'step': self.step
        ]
        if isinstance(self.mcts, mcts_get('EvaluationMCTSBase')):
            config['network_cls'] = [self.mcts.mixture_network.__class__.__name__]
        else:
            config['network_cls'] = [
                self.mcts.policy_network.__class__.__name__,
                self.mcts.rollout_network.__class__.__name__
            ]
        return config

    @classmethod
    def from_config(cls, config):
        mcts_cls = mcts_get(config['mcts_cls_name'])
        mcts_config = config['mcts_config']
        if issubclass(mcts_cls, mcts_get('EvaluationMCTSBase')):
            config_path, weight_path = cls.get_mixture_network_path(folder)
            mixture_network = network_get(config['network_cls'][0]).load_config(config_path)
            mixture_network.load_weights(weight_path)
            networks = {'mixture_network': mixture_network}
        else:
            policy_config_path, policy_weight_path = cls.get_policy_network_path(folder)
            policy_network = network_get(config['network_cls'][0]).load_config(policy_config_path)
            policy_network.load_weights(policy_weight_path)
            rollout_config_path, rollout_weight_path = cls.get_rollout_network_path(folder)
            rollout_network = network_get(config['network_cls'][1]).load_config(rollout_config_path)
            rollout_network.load_weights(rollout_weight_path)
            networks = {'policy_network': policy_network, 'rollout_network': rollout_network}
        mcts, boards = mcts_cls.from_config(mcts_config, **networks)
        sf = cls(mcts, config['number'], config['batch_size'], False)
        sf.index = config['index']
        sf.boards = boards
        sf.step = config['step']
        return sf


def evaluator():
    pass

def generator():
    pass

def trainer():
    pass

def main_loop():
    pass
