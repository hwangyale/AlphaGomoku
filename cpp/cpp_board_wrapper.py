from ..global_constants import *
from .swig_board import board as cpp_board

class CPPBoard(object):
    def __init__(self, history=[]):
        cpp_vector = cpp_board.IntVector([r*BOARD_SIZE+c for r, c in history])
        self.board = cpp_board.Board(cpp_vector)

    def move(self, pos):
        r, c = pos
        self.board.move(r*BOARD_SIZE+c)

    def get_positions(self, is_player, gomoku_type):
        return [(p // BOARD_SIZE, p % BOARD_SIZE) for p in self.board.get_positions(is_player, gomoku_type)]

    def vct(self, max_depth=20, max_time=1):
        pos = cpp_board.vct(self.board, max_depth, max_time)
        return pos // BOARD_SIZE, pos % BOARD_SIZE

    def __str__(self):
        _board = self.board.get_board()
        board_string = ''
        for row in range(BOARD_SIZE):
            s = ' '.join(str(_board[row*BOARD_SIZE+col]) for col in range(BOARD_SIZE))
            board_string += s + '\n'
        return board_string.replace('0', '_')
