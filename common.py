from random import random
from .global_constants import *

flatten = lambda x, y=None: x[0] * BOARD_SIZE + x[1] if y is None else x * BOARD_SIZE + y
unflatten = lambda x: (x // BOARD_SIZE, x % BOARD_SIZE)
tolist = lambda x: x if isinstance(x, list) else [x]
tosingleton = lambda x: x[0] if len(x) == 1 else x
player_map = lambda x: WHITE if x == BLACK else BLACK

hor_move = lambda act, d: (act[0], act[1]+d)
ver_move = lambda act, d: (act[0]+d, act[1])
dia_move = lambda act, d: (act[0]+d, act[1]+d)
bac_move = lambda act, d: (act[0]+d, act[1]-d)

move_functions = [hor_move, ver_move, dia_move, bac_move]

def multinomial_sampling(probs):
    s = random()
    if s <= probs[0]:
        return 0
    for i, p in enumerate(probs[1:], 1):
        s -= probs[i - 1]
        if s <= probs[i]:
            return i
    raise Exception('the sum of `probs` != 1: {}'.format(sum(probs)))
