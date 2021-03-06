__all__ = ['get_resnet']
from collections import defaultdict
import numpy as np
import keras.backend as K
import keras.layers as KL
import keras.engine as KE
import keras.initializers as KI
import keras.regularizers as KR
from .layers import Unitization

AXIS = 1 if K.image_data_format() == 'channels_first' else -1

class GetLayer(object):
    def __init__(self):
        self.cls_counts = defaultdict(int)

    def __call__(self, cls, *args, **kwargs):
        if not isinstance(cls, type):
            raise Exception('{} is not a class'.format(cls))
        if not issubclass(cls, KL.Layer):
            raise Exception('{} is not a Layer'.format(cls))
        kwargs['name'] = '{}_{}'.format(cls.__name__, self.cls_counts[cls])
        self.cls_counts[cls] += 1
        return cls(*args, **kwargs)


def get_resnet(input_shape, stack_nb, weight_decay=0.0005):
    inputs = KL.Input(shape=input_shape, name='inputs')
    get_layer = GetLayer()
    def residual_block(_tensor, filters):
        increase_filter = filters[0] != filters[1]
        output_filters = filters[1]
        if increase_filter:
            projection = get_layer(
                KL.Conv2D,
                output_filters,
                kernel_size=(1, 1),
                strides=(1, 1),
                padding='same',
                kernel_initializer=KI.he_normal(),
                kernel_regularizer=KR.l2(weight_decay)
            )(_tensor)
        else:
            projection = _tensor

        _tensor = get_layer(KL.BatchNormalization, axis=AXIS)(_tensor)
        _tensor = get_layer(KL.Activation, 'relu')(_tensor)
        _tensor = get_layer(
            KL.Conv2D,
            output_filters,
            kernel_size=(3, 3),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            kernel_regularizer=KR.l2(weight_decay)
        )(_tensor)

        _tensor = get_layer(KL.BatchNormalization, axis=AXIS)(_tensor)
        _tensor = get_layer(KL.Activation, 'relu')(_tensor)
        _tensor = get_layer(
            KL.Conv2D,
            output_filters,
            kernel_size=(3, 3),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            kernel_regularizer=KR.l2(weight_decay)
        )(_tensor)

        return get_layer(KL.Add)([_tensor, projection])

    tensor = get_layer(
        KL.Conv2D,
        filters=16,
        kernel_size=(3, 3),
        strides=(1, 1),
        padding='same',
        kernel_initializer=KI.he_normal(),
        kernel_regularizer=KR.l2(weight_decay)
    )(inputs)

    for _ in range(0, stack_nb):
        tensor = residual_block(tensor, [16, 16])

    tensor = residual_block(tensor, [16, 32])
    for _ in range(1, stack_nb):
        tensor = residual_block(tensor, [32, 32])

    tensor = residual_block(tensor, [32, 64])
    for _ in range(1, stack_nb):
        tensor = residual_block(tensor, [64, 64])

    tensor = get_layer(KL.BatchNormalization, axis=AXIS)(tensor)
    tensor = get_layer(KL.Activation, 'relu')(tensor)
    # tensor = get_layer(KL.GlobalAveragePooling2D)(tensor)
    return inputs, tensor

def get_unitized_resnet(input_shape, stack_nb, weight_decay=0.0005):
    inputs = KL.Input(shape=input_shape, name='inputs')
    get_layer = GetLayer()
    def residual_block(_tensor, filters):
        increase_filter = filters[0] != filters[1]
        output_filters = filters[1]
        if increase_filter:
            projection = get_layer(
                KL.Conv2D,
                output_filters,
                kernel_size=(1, 1),
                strides=(1, 1),
                padding='same',
                kernel_initializer=KI.he_normal(),
                kernel_regularizer=KR.l2(weight_decay)
            )(_tensor)
        else:
            projection = _tensor

        _tensor = get_layer(Unitization, axis=AXIS)(_tensor)
        _tensor = get_layer(KL.Activation, 'relu')(_tensor)
        _tensor = get_layer(
            KL.Conv2D,
            output_filters,
            kernel_size=(3, 3),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            kernel_regularizer=KR.l2(weight_decay)
        )(_tensor)

        _tensor = get_layer(Unitization, axis=AXIS)(_tensor)
        _tensor = get_layer(KL.Activation, 'relu')(_tensor)
        _tensor = get_layer(
            KL.Conv2D,
            output_filters,
            kernel_size=(3, 3),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            kernel_regularizer=KR.l2(weight_decay)
        )(_tensor)

        return get_layer(KL.Add)([_tensor, projection])

    tensor = get_layer(
        KL.Conv2D,
        filters=16,
        kernel_size=(3, 3),
        strides=(1, 1),
        padding='same',
        kernel_initializer=KI.he_normal(),
        kernel_regularizer=KR.l2(weight_decay)
    )(inputs)

    for _ in range(0, stack_nb):
        tensor = residual_block(tensor, [16, 16])

    tensor = residual_block(tensor, [16, 32])
    for _ in range(1, stack_nb):
        tensor = residual_block(tensor, [32, 32])

    tensor = residual_block(tensor, [32, 64])
    for _ in range(1, stack_nb):
        tensor = residual_block(tensor, [64, 64])

    tensor = get_layer(Unitization, axis=AXIS)(tensor)
    tensor = get_layer(KL.Activation, 'relu')(tensor)
    # tensor = get_layer(KL.GlobalAveragePooling2D)(tensor)
    return inputs, tensor
