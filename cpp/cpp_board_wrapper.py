from ..global_constants import *
from ..common import *
from .swig_board import board as cppBoard

class CPPBoard(object):
    def __init__(self, initialize=True):
        if initialize:
            self.cpp_board = cppBoard.Board()

    def move(self, action):
        self.cpp_board.move(flatten(action))

    def get_positions(self, is_player, gomoku_type):
        return [unflatten(act) for act in self.cpp_board.get_positions(is_player, gomoku_type)]

    def vct(self, max_depth=20, max_time=1):
        return unflatten(cpp_board.vct(self.cpp_board, max_depth, max_time))

    @property
    def is_over(self):
        return self.cpp_board.is_over

    @property
    def player(self):
        return self.cpp_board.player

    @property
    def winner(self):
        winner = self.cpp_board.winner
        if winner < 0:
            return None
        else:
            return winner

    @property
    def step(self):
        return self.cpp_board.step

    @property
    def history(self):
        history = self.cpp_board.history
        return [unflatten(history[i]) for i in range(self.step)]

    @property
    def key(self):
        return self.cpp_board.zobristKey

    def get_board(self):
        _board = self.board.get_board()
        board = []
        for row in range(BOARD_SIZE):
            board.append([_board[flatten(row, col)] for col in range(BOARD_SIZE)])
        return board

    def copy(self):
        new_board = self.__class__(False)
        new_board.cpp_board = cppBoard.Board(self.cpp_board)
        return new_board

    def __str__(self):
        board = self.get_board()
        board_string = ''
        for row in board:
            s = ' '.join([str(col) for col in row])
            board_string += s + '\n'
        return board_string.replace('0', '_')
