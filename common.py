from .global_constants import *

flatten = lambda x, y=None: x[0] * BOARD_SIZE + x[1] if y is None else x * BOARD_SIZE + y
unflatten = lambda x: (x // BOARD_SIZE, x % BOARD_SIZE)
tolist = lambda x: x if isinstance(x, list) else [x]
tosingleton = lambda x: x[0] if len(x) == 1 else x
player_map = lambda x: WHITE if x == BLACK else BLACK
