from ..neural_networks.keras.train import PrePolicyTrainer
from ..neural_networks.keras.train import PreValueTrainer

def run(trainer_cls, network, network_index,
        data_file, **kwargs):
    network_name = '{}_{}'.format(network.name, network_index)
    while True:
        try:
            from_cache = {'y': True, 'n': False}[input('from cache?[y/n]')]
        except:
            pass
    if from_cache:
        trainer = trainer_cls.from_cache(network_name)
        trainer.train(True)
    else:
        trainer = trainer_cls(network, network_name, data_file,
                              **kwargs)
        trainer.train(True)
