from keras.callbacks import LearningRateScheduler
from AlphaGomoku.neural_networks.keras.train import PrePolicyTrainer
from AlphaGomoku.neural_networks.keras.policy import ResNetPolicy
from AlphaGomoku.train.common import get_trainer
from AlphaGomoku.data import get_data_file

def main():
    resnet_policy = ResNetPolicy(stack_nb=1)
    resnet = resnet_policy.network
    trainer = get_trainer(PrePolicyTrainer, resnet, 0, get_data_file('pre', 0), 
                          batch_size=128, epochs=10, verbose=1)
    def scheduler(epoch):
        if epoch <= 60:
            return 0.05
        if epoch <= 120:
            return 0.01
        if epoch <= 160:
            return 0.002
        return 0.0004
    callbacks = [LearningRateScheduler(scheduler)]
    trainer.train(True, 0.9, callbacks=callbacks)

if __name == '__main__':
    main()
