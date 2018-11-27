__all__ = ['KerasResNetPolicy', 'KerasResNetValue', 'KerasResNetMixture']
import tensorflow as tf
import keras.backend.tensorflow_backend as KTF
from .policy import *
from .value import *
from .mixture import *

config = tf.ConfigProto()
config.gpu_options.per_process_gpu_memory_fraction = 0.9
# config.gpu_options.allow_growth = True
session = tf.Session(config=config)
KTF.set_session(session)

KerasResNetPolicy = ResNetPolicy
KerasResNetValue = ResNetValue
KerasResNetMixture = ResNetMixture
