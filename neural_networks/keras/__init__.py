import tensorflow as tf
import keras.backend.tensorflow_backend as KTF
from ...utils.generic_utils import deserialize_object
from .layers import *
from .models import *
from .policy import *
from .value import *
from .mixture import *
from .train import *

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

def get(config, *args, **kwargs):
    glos = globals()
    return deserialize_object(config, glos, *args, **kwargs)
