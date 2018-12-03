import os
import numpy as np
import keras.optimizers as KO
import keras.callbacks as KC
from ...utils.json_utils import json_dump_tuple, json_load_tuple
from ...temp import get_cache_folder, remove_folder
from .weights import get_config_file, get_weight_file

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
    def __init__(self, save_function):
        self.save_function = save_function

    def on_train_begin(self, logs=None):
        self.save_function()

    def on_epoch_end(self, logs=None):
        self.save_function()

class Trainer(object):
    def __init__(self, network, index, data_file, **kwargs):
        self.network = network
        self.index = index
        self.data_file = data_file
        self.optimizer_params = {
            'class_name': kwargs.get('optimizer', KO.SGD.__name__),
            'config': kwargs.get('optimizer_config',
                                 {'lr': 0.01, 'momentum': 0.9, 'nesterov': True})
        }
        self.compile_params = {
            'loss': self.get_loss(),
            'metrics': kwargs.get('metrics', ['acc']),
            'loss_weights': self.get_loss_weights()
        }
        self.train_params = {
            'steps_per_epoch': kwargs.get('steps_per_epoch', 128),
            'epochs': kwargs.get('epochs', 16),
            'verbose': kwargs.get('verbose', 1),
            'initial_epoch': kwargs.get('initial_epoch', 0)
        }
        self.compiled = False

    def compile(self):
        optimizer = KO.get(self.optimizer_params)
        self.network.compile(optimizer=optimizer, **self.optimizer_params)
        self.compiled = True

    def train(self, cache=True):
        if not self.compiled:
            self.compile()
        generator = self.get_generator()
        validation_data = self.get_validation_data()
        kwargs = {'generator': generator, 'validation_data': validation_data}
        kwargs.update(self.train_params)
        if cache:
            folder = self.get_cache_folder()
            json_dump_tuple(self.optimizer_params, os.path.join())
        return self.network.fit_generator(**kwargs)

    @classmethod
    def from_cache(self):

    def get_loss(self):
        raise Exception('Not implemented')

    def get_loss_weights(self):
        raise Exception('Not implemented')

    def get_generator(self):
        raise Exception('Not implemented')

    def get_validation_data(self):
        return None

    def get_cache_folder(self):
        raise Exception('Not implemented')

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
