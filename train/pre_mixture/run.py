from keras.callbacks import LearningRateScheduler, Callback
from AlphaGomoku.neural_networks.keras.train import PreMixtureTrainer
from AlphaGomoku.neural_networks.keras.mixture import ResNetMixture
from AlphaGomoku.neural_networks.keras.weights import get_config_file
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.train.common import get_trainer
from AlphaGomoku.data import get_data_file
from AlphaGomoku.utils.json_utils import json_dump_tuple

class Stop(Callback):
    def __init__(self, val_value_loss, val_distribution_acc):
        self.val_value_loss = val_value_loss
        self.val_distribution_acc = val_distribution_acc

    def on_epoch_end(self, epoch, logs=None):
        if logs['val_value_loss'] <= self.val_value_loss \
                and logs['val_distribution_acc'] >= self.val_distribution_acc:
            self.model.stop_training = True

def main(index):
    resnet_mixture = ResNetMixture(stack_nb=2)
    resnet = resnet_mixture.network
    trainer = get_trainer(PreMixtureTrainer, resnet, 0, get_data_file('pre', 0),
                          batch_size=128, epochs=200, verbose=1)
    def scheduler(epoch):
        if epoch <= 60:
            return 0.05
        if epoch <= 120:
            return 0.01
        if epoch <= 160:
            return 0.002
        return 0.0004
    callbacks = [LearningRateScheduler(scheduler), Stop(0.16, 0.5)]

    config_file_path = get_config_file('pre', resnet.name, index)
    weight_file_path = get_weight_file('pre', resnet.name, index)

    trainer.train(True, 0.9, callbacks=callbacks)

    json_dump_tuple(resnet.get_config(), config_file_path)
    resnet.save_weights(weight_file_path)

if __name__ == '__main__':
    main(1)
