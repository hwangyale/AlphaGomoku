__all__ = ['get_network']
from .keras import *

def get_network(function, network_type, backend, *args, **kwargs):
    if function not in ['policy', 'value', 'mixture']:
        raise Exception('Unknown `function`: {}'.format(function))

    if network_type not in ['resnet']:
        raise Exception('Unknown `network_type`: {}'.format(network_type))

    if backend not in ['keras']:
        raise Exception('Unknown backend: {}'.format(backend))

    function = function[0].capitalize() + function[1:]
    network_type = {
        'resnet': 'ResNet'
    }
    cls_name = '{}{}'.format(network_type, function)
    if backend == 'keras':
        return globals()['Keras{}'.format(cls_name)](*args, **kwargs)
