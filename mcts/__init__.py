__all__ = ['get']
from .rollout_mcts import *
from .evaluation_mcts import *
from .rl_evaluation_mcts import *

all = globals()

def get(obj_name):
    return all[obj]
