import keras.optimizers as KO
import keras.callbacks as KC
import keras.engine as KE


def optimizer_wrapper(optimizer):
    if not isinstance(optimizer, KO.Optimizer):
        raise Exception('`optimizer` must be an instance of `Optimizer`')

    def load_weights(self, file_path):
        npz_file = np.load(file_path)
        self.set_weights([npz_file[str(i)] for i in range(len(npz_file.files))])

    def save_weights(self, file_path):
        weights = self.get_weights()
        np.savez(file_path, **{str(i): weight for i, weight in enumerate(weights)})

    optimizer.load_weights = lambda file_path : load_weights(optimizer, file_path)
    optimizer.save_weights = lambda file_path : save_weights(optimizer, file_path)
    return optimizer


class CacheCallback(KC.Callback):
    def __init__(self, begin_save, epoch_save, folder):
        self.begin_save = begin_save
        self.epoch_save = epoch_save
        self.folder = folder

    def on_train_begin(self, logs=None):
        self.begin_save()

    def on_epoch_end(self, epoch, logs=None):
        self.epoch_save()

    def on_train_end(self, logs=None):
        remove_folder(self.folder)
