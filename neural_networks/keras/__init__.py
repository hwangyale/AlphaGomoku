import tensorflow as tf
import keras.backend.tensorflow_backend as KTF

config = tf.ConfigProto()
config.gpu_options.per_process_gpu_memory_fraction = 0.5
config.gpu_options.allow_growth = True
session = tf.Session(config=config)
KTF.set_session(session)
