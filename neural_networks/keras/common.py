__all__ = ['register_object', 'register_objects', 'register_layers']
import keras.backend as K
import keras.layers as KL
from ...common import *

def register_object(object):
    get_custom_objects().update(object)

def register_objects(objects, base_class):
    objects = [obj for obj in objects
               if isinstance(obj, type) and issubclass(obj, base_class)]
    for object in objects:
        register_object({object.__name__: object})

def register_layers(layer_classes):
    register_objects(tolist(layer_classes), KL.Layer)
