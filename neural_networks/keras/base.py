__all__ = ['register_layers']
from .common import register_objects

def register_layers(layer_classes):
    register_objects(tolist(layer_classes), KL.Layer)
