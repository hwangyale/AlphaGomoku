from ..utils.generic_utils import deserialize_object
from .rollout_mcts import *
from .evaluation_mcts import *
from .rl_evaluation_mcts import *

def get(config, *args, **kwargs):
    glos = globals()
    return deserialize_object(config, glos, *args, **kwargs)
