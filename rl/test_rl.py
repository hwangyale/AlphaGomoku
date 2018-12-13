from AlphaGomoku.rl.rl import EvaluationMainLoop
from AlphaGomoku.mcts.evaluation_mcts import EvaluationMCTS
from AlphaGomoku.mcts.rl_evaluation_mcts import RLEvaluationMCTS
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file

mixture = get_network('mixture', 'resnet', 'keras', stack_nb=2)
mixture.load_weights(get_weight_file('pre', mixture.network.name, 0))

kwargs = {
    'self_play_mcts_config': {
        'traverse_time': 10, 'c_puct': None,
        'thread_number': 1, 'delete_threshold': 10
    },
    'self_play_number': 10,
    'self_play_batch_size': 10,
    'self_play_cache_step': 10,
    'evaluate_mcts_config': {
        'traverse_time': 10, 'c_puct': None,
        'thread_number': 1, 'delete_threshold': 10
    },
    'evaluate_number': 5,
    'evaluate_win_ratio': 0.55,
    'train': {
        'batch_size': 128,
        'epochs': 1,
        'verbose': 1
    }
}
mainLoop = EvaluationMainLoop(mixture, **kwargs)
if False:
# if {'y': True, 'n': False}[input('from cache[y/n]: ')]:
    mainLoop.run(True)
else:
    mainLoop.run(False)
