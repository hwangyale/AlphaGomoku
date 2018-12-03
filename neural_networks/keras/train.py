import numpy as np
import keras.optimizers as KO
import keras.callbacks as KC

def optimizer_wrapper(optimizer):
    if not isinstance(optimizer, KO.Optimizer):
        raise Exception('`optimizer` must be an instance of `Optimizer`')

    def load_config(self, )

    def load_weights(self, file_path):
        npz_file = np.load(file_path)
        self.set_weights([npz_file['arr_{}'.format(i)]
                          for i in range(len(npz_file))])

    def save_weights(self, file_path)
        weights = self.get_weights()
        np.savez(file_path, *weights)

    optimizer.load_weights = lambda file_path : load_weights(optimizer, file_path)
    optimizer.save_weights = lambda file_path : save_weights(optimizer, file_path)
    return optimizer

class CacheCallback(KC.Callback):
    def __init__(self, config_file, weight_file, optimizer_file):


def train(batch, epochs, verbose=1, )

def train_policy(network, history_action_pairs, batch_size, epochs, ):

def train_value(network, history_value_pairs):

def train_mixture(network, tuples, ):
