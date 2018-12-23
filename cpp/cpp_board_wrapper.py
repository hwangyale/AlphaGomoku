import time
from ..global_constants import *
from ..common import *
from .swig_board import board as cppBoard


class CPPBoard(object):
    def __init__(self, initialize=True, history=[]):
        if initialize:
            self.cpp_board = cppBoard.Board()
            for action in history:
                self.move(action)

    def move(self, action):
        self.cpp_board.move(flatten(action))

    def get_actions(self, is_player, gomoku_type):
        actions = self.cpp_board.get_actions(is_player, gomoku_type)
        return [unflatten(act) for act in self.cpp_board.get_actions(is_player, gomoku_type)]

    def vct(self, max_depth=20, max_time=1, iterative_deepening_mode=False):
        if iterative_deepening_mode and max_depth >= 6:
            depth = 6
            action = -1
            start = time.time()
            while action < 0 and depth <= max_depth \
                and time.time() - start < max_time:
                action = self.cpp_board.Vct(depth, max_time-(time.time()-start))
                depth += 2
        else:
            action = self.cpp_board.Vct(max_depth, max_time)
        if action < 0:
            return None
        return unflatten(action)

    def defend_vct(self, action, max_depth, max_time):
        copy_cpp = cppBoard.Board(self.cpp_board)
        copy_cpp.move(flatten(action))
        return copy_cpp.Vct(max_depth, max_time) < 0

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
        history = self.cpp_board.get_history()
        return [unflatten(history[i]) for i in range(self.step)]

    @property
    def key(self):
        return self.cpp_board.zobristKey

    def get_board(self):
        _board = self.cpp_board.get_board()
        board = []
        for row in range(BOARD_SIZE):
            board.append([_board[flatten(row, col)] for col in range(BOARD_SIZE)])
        return board

    def copy(self):
        new_board = CPPBoard(False)
        new_board.cpp_board = cppBoard.Board(self.cpp_board)
        return new_board

    def __str__(self):
        board = self.get_board()
        board_string = ''
        for i, row in enumerate(board, 1):
            s = '{:2d} '.format(i)
            s += ' '.join([str(col) if col != EMPTY else '_' for col in row])
            board_string += s + '\n'
        board_string += ' ' * 3
        board_string += ' '.join([str(i) if i < 10 else '1' for i in range(1, BOARD_SIZE+1)]) + '\n'
        board_string += ' ' * 3
        board_string += ' '.join([' ' if i < 10 else str(i-10) for i in range(1, BOARD_SIZE+1)]) + '\n'
        return board_string
