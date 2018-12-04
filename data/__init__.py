__all__ = ['get_data_file']
import os

path = os.path.dirname(os.path.realpath(__file__))

def get_data_file(pattern, index):
    if pattern not in ['pre', 'mcts']:
        raise Exception('Unknwon pattern: {}'.format(pattern))
    return os.path.join(path, 'pattern', '{}.json'.(index))
