__all__ = ['get_cache_folder', 'get_file_path',
           'remove_folder', 'get_temp_weight_file']
import os
import numpy as np

path = os.path.dirname(os.path.realpath(__file__))
def get_cache_folder(folder_name):
    folder_path = os.path.join(path, folder_name)
    if not os.path.exists(folder_path):
        os.mkdir(folder_path)
    return folder_path

def get_path(folder_path, file_name):
    return os.path.join(folder_path, file_name)

def remove_folder(folder_path):
    if not os.path.exists(folder_path):
        return
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            os.remove(os.path.join(root, file))
    for root, dirs, files in os.walk(folder_path, False, lambda x: x):
        for dir in dirs:
            os.rmdir(os.path.join(root, dir))
    os.rmdir(root)

def get_temp_weight_file(weight_index=None):
    folder = get_cache_folder('temp_weights')
    if weight_index is None:
        while True:
            weight_index = np.random.randint(2**31)
            weight_file = os.path.join(folder, '{}.npz'.format(weight_index))
            if not os.path.exists(weight_file):
                return weight_index, weight_file
    weight_file = os.path.join(folder, '{}.npz'.format(weight_index))
    if not os.path.exists(weight_file):
        raise Exception('Not found weight file: {}'.format(weight_file))
    return weight_file

def remove_temp_weight_file(weight_index):
    weight_file = get_temp_weight_file(weight_index)
    os.remove(weight_file)
