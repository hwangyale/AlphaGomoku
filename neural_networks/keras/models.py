from collections import defaultdict
import numpy as np
import keras.backend as K
import keras.layers as KL
import keras.engine as KE
import keras.initializers as KI
import keras.regularizers as KR

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


def get_resnet(input_shape, stack_nb):
    inputs = KL.Input(shape=input_shape, name='inputs')
    get_layer = GetLayer()
    def residual_block(_tensor, filters):
        increase_filter = filters[0] != filters[1]
        output_filters = filters[1]
        if increase_filter:
            first_stride = (2, 2)
            projection = get_layer(
                KL.Conv2D,
                output_filters,
                kernel_size=(1, 1),
                strides=(2, 2),
                padding='same',
                kernel_initializer=KI.he_normal(),
                kernel_regularizer=KR.l2(weight_decay)
            )(_tensor)
        else:
            first_stride = (1, 1)
            projection = _tensor

        _tensor = KL.BatchNormalization(axis=AXIS)(_tensor)
        _tensor = KL.Activation('relu')(_tensor)

        _tensor = KL.Conv2D(
            output_filters,
            kernel_size=(3, 3),
            strides=first_stride,
            padding='same',
            kernel_initializer=KI.he_normal(),
            kernel_regularizer=KR.l2(weight_decay),
            name=get_conv_name(convolution_count)
        )(_tensor)
        _tensor = KL.BatchNormalization(axis=AXIS)(_tensor)
        _tensor = KL.Activation('relu')(_tensor)

        _tensor = KL.Conv2D(
            output_filters,
            kernel_size=(3, 3),
            strides=(1, 1),
            padding='same',
            kernel_initializer=KI.he_normal(),
            kernel_regularizer=KR.l2(weight_decay),
            name=get_conv_name(convolution_count)
        )(_tensor)

        return KL.add([_tensor, projection])

    tensor = KL.Conv2D(
        filters=16,
        kernel_size=(3, 3),
        strides=(1, 1),
        padding='same',
        kernel_initializer=KI.he_normal(),
        kernel_regularizer=KR.l2(weight_decay),
        name=get_conv_name(convolution_count)
    )(inputs)

    for _ in range(0, stack_nb):
        tensor = residual_block(tensor, [16, 16])

    tensor = residual_block(tensor, [16, 32])
    for _ in range(1, stack_nb):
        tensor = residual_block(tensor, [32, 32])

    tensor = residual_block(tensor, [32, 64])
    for _ in range(1, stack_nb):
        tensor = residual_block(tensor, [64, 64])

    tensor = KL.BatchNormalization(axis=AXIS)(tensor)
    tensor = KL.Activation('relu')(tensor)
    tensor = KL.GlobalAveragePooling2D()(tensor)
    return inputs, tensor
