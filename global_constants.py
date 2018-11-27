__all__ = ['BOARD_SIZE', 'STONES', 'EMPTY', 'BLACK', 'WHITE',
           'DRAW', 'OPEN_FOUR', 'FOUR', 'OPEN_THREE', 'THREE',
           'OPEN_TWO', 'TWO', 'BINARY_HEPLERS', 'ATTACK_VCT_DEPTH',
           'ATTACK_VCT_TIME', 'DEFEND_VCT_DEPTH', 'DEFEND_VCT_TIME',
           'FLOATX', 'EPSILON']
BOARD_SIZE = 15
STONES = 225
EMPTY = 0
BLACK = 1
WHITE = 2
DRAW = 3

OPEN_FOUR = 1
FOUR = 2
OPEN_THREE = 3
THREE = 4
OPEN_TWO = 5
TWO = 6
BINARY_HEPLERS = [2**i for i in range(BOARD_SIZE**2)]

ATTACK_VCT_DEPTH = 14
ATTACK_VCT_TIME = 1
DEFEND_VCT_DEPTH = 6
DEFEND_VCT_TIME = 0.1

MCTS_BOUND = 3
MCTS_VCT_DEPTH = 12
MCTS_VCT_TIME = 0.3

FLOATX = 'float32'
EPSILON = 1e-7
