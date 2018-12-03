__all__ = ['get_cache_folder', 'get_file_path']
import os

path = os.path.dirname(os.path.realpath(__file__))
def get_cache_folder(folder_name):
    folder_path = os.path.join(path, folder_name)
    if not os.path.exists(folder_path):
        os.mkdir(folder_path)
    return folder_path

def get_path(folder_path, file_name):
    return os.path.join(folder_path, file_name)

def remove_folder(folder_path):
    def recursive(_folder):
