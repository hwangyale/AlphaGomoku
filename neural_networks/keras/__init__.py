__all__ = ['KerasResNetPolicy', 'KerasResNetValue', 'KerasResNetMixture',
           'KerasUnitizedResNetPolicy', 'KerasUnitizedResNetValue',
           'KerasUnitizedResNetMixture', 'get']
import tensorflow as tf
import keras.backend.tensorflow_backend as KTF
from .layers import *
from .models import *
from .policy import *
from .value import *
from .mixture import *

config = tf.ConfigProto()
# config.gpu_options.per_process_gpu_memory_fraction = 0.9
config.gpu_options.allow_growth = True
session = tf.Session(config=config)
KTF.set_session(session)

KerasResNetPolicy = ResNetPolicy
KerasResNetValue = ResNetValue
KerasResNetMixture = ResNetMixture

KerasUnitizedResNetPolicy = UnitizedResNetPolicy
KerasUnitizedResNetValue = UnitizedResNetValue
KerasUnitizedResNetMixture = UnitizedResNetMixture

all = globals()
def get(obj_name):
    return all[obj_name]
