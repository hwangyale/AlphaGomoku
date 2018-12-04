from keras.callbacks import LearningRateScheduler
from AlphaGomoku.neural_networks.keras.train import PrePolicyTrainer
from AlphaGomoku.neural_networks.keras.policy import ResNetPolicy
from AlphaGomoku.neural_networks.keras.weights import get_config_file
from AlphaGomoku.neural_networks.keras.weights import get_weight_file
from AlphaGomoku.train.common import get_trainer
from AlphaGomoku.data import get_data_file
from AlphaGomoku.utils.json_utils import json_dump_tuple

def main():
    resnet_policy = ResNetPolicy(stack_nb=1)
    resnet = resnet_policy.network
    trainer = get_trainer(PrePolicyTrainer, resnet, 0, get_data_file('pre', 0),
                          batch_size=128, epochs=200, verbose=1)
    def scheduler(epoch):
        if epoch <= 60:
            return 0.05
        if epoch <= 120:
            return 0.01
        if epoch <= 160:
            return 0.002
        return 0.0004
    callbacks = [LearningRateScheduler(scheduler)]

    config_file_path = get_config_file('pre', resnet.name, 0)
    weight_file_path = get_weight_file('pre', resnet.name, 0)

    trainer.train(True, 0.9, callbacks=callbacks)

    json_dump_tuple(resnet.get_config(), config_file_path)
    resnet.save_weights(weight_file_path)

if __name__ == '__main__':
    main()
