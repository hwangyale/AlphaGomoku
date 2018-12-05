import numpy as np
import tensorflow as tf
import keras.backend as K
import keras.layers as KL
import keras.initializers as KI
import keras.regularizers as KR
import keras.constraints as KC
from .common import register_layers


class Unitization(KL.BatchNormalization):
    def __init__(self, norm_initializer='zeros',
                 norm_initial_value=0.0, **kwargs):
        self.norm_initializer = norm_initializer
        self.norm_initial_value = norm_initial_value
        super(Unitization, self).__init__(**kwargs)

    def build(self, input_shape):
        dim = input_shape[self.axis]
        self.alpha = self.add_weight(shape=(dim, ),
                                     name='alpha',
                                     initializer=self.norm_initializer,
                                     trainable=True)
        if self.norm_initial_value is not None:
            value = np.ones_like(K.get_value(self.alpha)) * self.norm_initial_value
            K.set_value(self.alpha, value)
        super(Unitization, self).build(input_shape)

    def call(self, inputs, training=None):
        input_shape = K.int_shape(inputs)
        ndim = len(input_shape)
        reduction_axes = list(range(len(input_shape)))
        del reduction_axes[self.axis]
        broadcast_shape = [1] * ndim
        broadcast_shape[self.axis] = input_shape[self.axis]

        def normalize_inference():
            broadcast_moving_mean = K.reshape(self.moving_mean,
                                              broadcast_shape)
            broadcast_moving_variance = K.reshape(self.moving_variance,
                                                  broadcast_shape)

            normalized_inputs = (inputs - broadcast_moving_mean) * \
                                tf.rsqrt(broadcast_moving_variance + self.epsilon)

            squared_norm = tf.reduce_sum(normalized_inputs**2,
                                         list(range(1, ndim)), True)

            if ndim > 2:
                shape_idxs = list(range(ndim))
                del shape_idxs[self.axis]
                if self.axis:
                    del shape_idxs[0]
                reduce_number = 1.0
                for idx in shape_idxs:
                    reduce_number *= input_shape[idx]
                squared_norm /= reduce_number**2

            norm_values = K.reshape(self.alpha, broadcast_shape)
            scale = norm_values * tf.rsqrt(squared_norm + self.epsilon) + (1-norm_values)
            if self.scale:
                scale *= K.reshape(self.gamma, broadcast_shape)
            normalized_inputs *= scale

            if self.center:
                normalized_inputs += K.reshape(self.beta, broadcast_shape)

            return normalized_inputs


        if training in {0, False}:
            normalized_inputs = normalize_inference()

        else:
            mean, variance = tf.nn.moments(inputs, reduction_axes,
                                           None, None, False)

            if K.backend() != 'cntk':
                sample_size = K.prod([K.shape(inputs)[axis]
                                      for axis in reduction_axes])
                sample_size = K.cast(sample_size, dtype=K.dtype(inputs))

                variance *= sample_size / (sample_size - (1.0 + self.epsilon))

            self.add_update([K.moving_average_update(self.moving_mean,
                                                     mean,
                                                     self.momentum),
                             K.moving_average_update(self.moving_variance,
                                                     variance,
                                                     self.momentum)],
                            inputs)

            mean = K.reshape(mean, broadcast_shape)
            variance = K.reshape(variance, broadcast_shape)
            normalized_inputs = (inputs - mean) * tf.rsqrt(variance + self.epsilon)

            squared_norm = tf.reduce_sum(normalized_inputs**2,
                                         list(range(1, ndim)), True)

            if ndim > 2:
                shape_idxs = list(range(ndim))
                del shape_idxs[self.axis]
                if self.axis:
                    del shape_idxs[0]
                reduce_number = 1.0
                for idx in shape_idxs:
                    reduce_number *= input_shape[idx]
                squared_norm /= reduce_number**2

            norm_values = K.reshape(self.alpha, broadcast_shape)
            scale = norm_values * tf.rsqrt(squared_norm + self.epsilon) + (1-norm_values)
            if self.scale:
                scale *= K.reshape(self.gamma, broadcast_shape)
            normalized_inputs *= scale

            if self.center:
                normalized_inputs += K.reshape(self.beta, broadcast_shape)

        return K.in_train_phase(normalized_inputs,
                                normalize_inference,
                                training=training)

    def get_config(self):
        config = {
            'norm_initializer': self.norm_initializer,
            'norm_initial_value': self.norm_initial_value,
        }
        base_config = super(Unitization, self).get_config()
        return dict(list(base_config.items()) + list(config.items()))

objects = [Unitization]
register_layers(objects)
