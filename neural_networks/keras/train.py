import numpy as np
import keras.optimizers as KO
import keras.callbacks as KC
from ...utils.json_utils import json_dump_tuple, json_load_tuple
from ...temp import get_cache_folder, remove_folder

def optimizer_wrapper(optimizer):
    if not isinstance(optimizer, KO.Optimizer):
        raise Exception('`optimizer` must be an instance of `Optimizer`')

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
    def __init__(self, config_file, weight_file,
                 optimizer_config_file, optimizer_weight_file,
                 train_config_file):
        self.config_file = config_file
        self.weight_file = weight_file
        self.optimizer_config_file = optimizer_config_file
        self.optimizer_weight_file = optimizer_weight_file
        self.train_config_file = train_config_file
        self.train_params = {}

    def set_train_params(self, **kwargs):
        self.train_params.update(kwargs)

    def on_train_begin(self, logs=None):
        self.save()

    def on_epoch_end(self, logs=None):
        self.train_params['initial_epoch'] += 1
        self.save()

    def save(self):
        config = self.model.get_config()
        json_dump_tuple(config, self.config_file)
        self.model.save_weights(self.weight_file)

        optimizer_config = self.optimizer.get_config()
        json_dump_tuple(optimizer_config, self.optimizer_config_file)
        self.optimizer.save_weights(self.optimizer_weight_file)

        json_dump_tuple(self.train_params, self.train_config_file)

def train(network, generator, optimizer, loss,
          batch_size=128, epochs=10, verbose=1,
          validation_data=None, **kwargs):
    optimizer = optimizer_wrapper(KO.get(optimizer))
    compile_setting = {'optimizer': optimizer, 'loss': loss}
    parameters = set(network.compile.__code__.co_varnames) \
                 & set(kwargs.keys())
    compile_setting.update({p: kwargs[p] for p in parameters})
    network.compile(**compile_setting)

    fit_setting = {'generator': generator, 'batch_size': batch_size,
                   'epochs': epochs, 'verbose': verbose,
                   'validation_data': validation_data}
    parameters = set(network.fit_generator.__code__.co_varnames) \
                 & set(kwargs.keys())
    fit_setting.update({p: kwargs[p] for p in parameters})
    fit_setting.setdefault('initial_epoch', 0)
    return network.fit_generator(**fit_setting)

def from_temp(network):


def train_policy(network, history_action_pairs, **kwargs):

def train_value(network, history_value_pairs, **kwargs):

def train_mixture(network, tuples, **kwargs):
