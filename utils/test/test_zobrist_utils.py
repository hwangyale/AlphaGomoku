from AlphaGomoku.global_constants import *
from AlphaGomoku.utils.zobrist_utils import *
from AlphaGomoku.common import unflatten
from AlphaGomoku.global_constants import *

print(get_single_zobrist_key(BLACK, (0, 0)))
print(get_single_zobrist_key(WHITE, (0, 0)))
print(hash_history([]))
print(hash_history([(0, 0), (8, 8), (9, 9), (10, 10)]))
print(hash_history([(9, 9), (10, 10), (0, 0), (8, 8)]))
print(hash_history([(8, 8), (0, 0), (9, 9), (10, 10)]))

for color in [BLACK, WHITE]:
    for action in range(BOARD_SIZE**2):
        get_single_zobrist_key(color, unflatten(action))
