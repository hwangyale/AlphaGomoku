__all__ = ['get_config_file', 'get_weight_file']
import os

path = os.path.dirname(os.path.realpath(__file__))
pre_train_path = os.path.join(path, 'pre_train')
if not os.path.exists(pre_train_path):
    os.mkdir(pre_train_path)
rl_train_path = os.path.join(path, 'rl_pre_train')
if not os.path.exists(rl_train_path):
    os.mkdir(rl_train_path)
zero_train_path = os.path.join(path, 'zero_train_path')
if not os.path.exists(zero_train_path):
    os.mkdir(zero_train_path)

def get_config_file(pattern, network_name, index):
    folder = {
        'pre': pre_train_path,
        'rl': rl_train_path,
        'zero': zero_train_path
    }.get(pattern, None)

    if folder is None:
        raise Exception('Unknown pattern: {}'.format(pattern))

    return os.path.join(folder, '{}_{}_config.json'.format(network_name, index))

def get_weight_file(pattern, network_name, index):
    folder = {
        'pre': pre_train_path,
        'rl': rl_train_path,
        'zero': zero_train_path
    }.get(pattern, None)

    if folder is None:
        raise Exception('Unknown pattern: {}'.format(pattern))

    return os.path.join(folder, '{}_{}_weights.npz'.format(network_name, index))
