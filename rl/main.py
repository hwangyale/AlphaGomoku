from AlphaGomoku.rl.rl import EvaluationMainLoop
from AlphaGomoku.mcts.evaluation_mcts import EvaluationMCTS
from AlphaGomoku.mcts.rl_evaluation_mcts import RLEvaluationMCTS
from AlphaGomoku.neural_networks import get_network
from AlphaGomoku.neural_networks.keras.weights import get_weight_file

def run():
    mixture = get_network('mixture', 'resnet', 'keras', stack_nb=2)
    # mixture.load_weights(get_weight_file('pre', mixture.network.name, 0))

    kwargs = {
        'self_play_mcts_config': {
            'traverse_time': 1000, 'c_puct': None,
            'thread_number': 1, 'delete_threshold': 10
        },
        'self_play_number': 1000,
        'self_play_batch_size': 40,
        'self_play_cache_step': 4,
        'evaluate_mcts_config': {
            'traverse_time': 1000, 'c_puct': None,
            'thread_number': 1, 'delete_threshold': 10
        },
        'evaluate_number': 25,
        'evaluate_win_ratio': 0.55,
        'train': {
            'batch_size': 128,
            'epochs': 100,
            'verbose': 1
        }
    }
    mainLoop = EvaluationMainLoop(mixture, **kwargs)
    if {'y': True, 'n': False}[input('from cache[y/n]: ')]:
        mainLoop.run(True)
    else:
        mainLoop.run(False)


if __name__ == '__main__':
    run()
