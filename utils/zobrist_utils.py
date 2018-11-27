__all__ = ['get_single_zobrist_key', 'get_zobrist_key', 'hash_history']
import os
import json
import numpy as np
from ..global_constants import *
from ..common import *

def get_zobrist_table():
    zobrist_table = {}
    for color in [BLACK, WHITE]:
        for action in range(BOARD_SIZE**2):
            zobrist_table[(color, action)] = np.random.randint(2**31 - 1) + 1
    return zobrist_table

def code_json_dict(zobrist_table):
    json_table = {}
    for (color, action), key in zobrist_table.items():
        json_table[(-1 if color == BLACK else 1) * (flatten(action) + 1)] = key
    return json_table

def decode_json_dict(json_table):
    zobrist_table = {}
    for key, value in json_table.items():
        key = int(key)
        color = BLACK if key < 0 else WHITE
        action = unflatten(abs(key) - 1)
        zobrist_table[(color, action)] = int(value)
    return zobrist_table

def load_zobrist_table():
    folder_path = '\\'.join(os.path.realpath(__file__).split('\\')[:-1])
    filepath = os.path.join(folder_path, 'data', 'zobrist_table.json')
    try:
        with open(filepath, 'r') as f:
            zobrist_table = decode_json_dict(json.load(f))
    except:
        zobrist_table = get_zobrist_table()
        with open(filepath, 'w') as f:
            json.dump(code_json_dict(zobrist_table), f)
    return zobrist_table

ZOBRIST_TABLE = load_zobrist_table()

def get_single_zobrist_key(color, action):
    return ZOBRIST_TABLE[(color, action)]

def get_zobrist_key(color, action, key=0):
    return key ^ get_single_zobrist_key(color, action)

def hash_history(history):
    key = 0
    color = BLACK
    for action in history:
        key ^= get_single_zobrist_key(color, action)
        color = player_map(color)
    return key
