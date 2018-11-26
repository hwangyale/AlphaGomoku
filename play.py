import numpy as np
from .global_constants import *
from .common import *
from .board import Board


class PlayerBase(object):
    def get_action(self, board):
        raise NotImplementedError('`get_action` has not been implemented')


class Human(PlayerBase):
    def __init__(self, check_vct=False, vct_depth=14, vct_time=30):
        self.check_vct = check_vct
        self.vct_depth = vct_depth
        self.vct_time = vct_time

    def get_action(self, board):
        if self.check_vct and board.step > 6:
            print('searching vct action......')
            action = board.vct(self.vct_depth, self.vct_time)
            if action is not None:
                print('get vct action!')
                return action
            else:
                print('failed! ', end='')
        while True:
            string = input('input action:\n')
            try:
                row, col = string.split()
                row = int(row)
                col = int(col)
                if board.check_legality((row - 1, col - 1)):
                    return (row - 1, col - 1)
            except:
                pass


class Game(object):
    def __init__(self, black, white, board_cls=Board, **kwargs):
        self.black = black
        self.white = white
        self.board = board_cls(**kwargs)

    def play_generator(self, verbose=0, hold_time=60):
        board = self.board
        player = self.black
        opponent = self.white
        if verbose:
            if not board.visualization:
                print(board)
            else:
                board.visualize(board.visualization_time)
        while not board.is_over:
            action = player.get_action(board)
            board.move(action)
            if verbose:
                print()
                if not board.visualization:
                    print(board)
                print('move: ({} {})'.format(action[0]+1, action[1]+1))
            yield player, action
            player, opponent = opponent, player
        winner = board.winner
        if winner == BLACK:
            print('black wins!')
        elif winner == WHITE:
            print('white wins!')
        else:
            print('draw!')

    def play(self, verbose=0, hold_time=60):
        for player, move in self.play_generator(verbose, hold_time):
            pass
