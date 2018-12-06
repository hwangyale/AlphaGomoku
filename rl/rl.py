import os
import numpy as np
from ..global_constants import *
from ..common import *
from ..board import Board
from ..mcts.evaluation_mcts import EvaluationMCTSBase
from ..mcts.rollout_mcts import RolloutMCTSBase
from ..utils.json_utils import json_dump_tuple, json_load_tuple


rng = np.random


class SelfPlay(object):
    def __init__(self, mcts, number, batch_size):
        self.mcts = mcts
        self.number = number
        self.batch_size = batch_size

    def run(self, cache_step=None, cache_folder='', board_cls=Board):
        mcts = self.mcts
        number = self.number
        batch_size = self.batch_size
        boards = [board_cls() for _ in range(number)]
        index = min(batch_size, number)
        play_indice = set(range(index))
        step = 0
        while len(play_indice) > 0:
            temp_indice = list(play_indice)
            temp_boards = [boards[idx] for idx in temp_indice]
            actions = tolist(mcts.mcts(temp_boards, verbose=0))
            for idx, board, action in zip(temp_indice, temp_boards, actions):
                board.move(action)
                if board.is_over:
                    play_indice.remove(idx)
                    if index < number:
                        play_indice.add(index)
                        index += 1
            step += 1
            if cache_step is not None and step % cache_step == 0:
                if isinstance(mcts, EvaluationMCTSBase):
                    config_path, weight_path = self.get_mixture_network_path(folder)
                    json_dump_tuple(mcts.mixture_network.get_config(), config_path)
                    mcts.mixture_network.save_weights(weight_path)
                else:
                    policy_config_path, policy_weight_path = self.get_policy_network_path(folder)
                    json_dump_tuple(mcts.policy.get_config(), policy_config_path)
                    mcts.policy.save_weights(policy_weight_path)
                    rollout_config_path, rollout_weight_path = self.get_rollout_network_path(folder)
                    json_dump_tuple(mcts.rollout.get_config(), rollout_config_path)
                    mcts.rollout.save(rollout_weight_path)

                json_dump_tuple(mcts.get_config(), self.get_mcts_path(folder))

    def load_and_run(self, folder):
        pass

    def get_mixture_network_path(self, folder):
        config_path = os.path.join(folder, 'mixture_network_config.json')
        weight_path = os.path.join(folder, 'mixture_network_weights.npz')
        return config_path, weight_path

    def get_policy_network_path(self, folder):
        policy_config_path = os.path.join(folder, 'policy_network_config.json')
        policy_weight_path = os.path.join(folder, 'policy_network_weights.npz')
        return policy_config_path, policy_weight_path

    def get_rollout_network_path(self, folder):
        rollout_config_path = os.path.join(folder, 'rollout_network_config.json')
        rollout_weight_path = os.path.join(folder, 'rollout_network_weights.npz')
        return rollout_config_path, rollout_weight_path

    def get_mcts_path(self, folder):
        return os.path.join(folder, 'mcts.json')

def evaluator():
    pass

def generator():
    pass

def trainer():
    pass

def main_loop():
    pass
