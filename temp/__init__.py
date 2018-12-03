__all__ = ['get_cache_folder', 'get_file_path', 'remove_folder']
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
    if not os.path.exists(folder_path):
        return
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            os.remove(os.path.join(root, file))
    for root, dirs, files in os.walk(folder_path, False, lambda x: x):
        for dir in dirs:
            os.rmdir(os.path.join(root, dir))
    os.rmdir(root)
