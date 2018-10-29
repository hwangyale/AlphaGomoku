__all__ = ['BOARD_SIZE', 'STONES', 'EMPTY', 'BLACK', 'WHITE',
           'DRAW', 'OPEN_FOUR', 'FOUR', 'OPEN_THREE', 'THREE',
           'OPEN_TWO', 'TWO', 'BINARY_HEPLERS']
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
