import time
from AlphaGomoku.cpp import CPPBoard

# board = cpp_board.Board(cpp_board.IntVector([112, 97, 127]))

# record = [
#     (7, 7), (6, 7), (6, 8), (7, 8), (6, 6), (6, 5),
#     (5, 6), (4, 6), (5, 5), (4, 4), (7, 6), (5, 9),
#     (8, 6), (9, 6), (8, 7), (8, 5)
# ]

record = [
    (7, 7), (7, 8), (6, 8), (6, 7), (5, 6), (5, 7),
    (4, 7), (4, 6), (3, 8), (2, 9), (5, 9), (4, 10),
    (8, 9), (9, 9), (4, 9), (7, 9)
]

# record += [
#     (8, 8), (9, 9), (8, 9), (8, 10), (5, 3), (5, 4),
#     (7, 5), (6, 4), (7, 4), (7, 3), (4, 8)
# ]

board = CPPBoard(history=record)
# print(board)
# print('player: {}'.format(board.player))
# for is_player in [True, False]:
#     print('is player: {}'.format(is_player))
#     for gt in range(1, 6):
#         actions = board.get_actions(is_player, gt)
#         if len(actions) != 0:
#             print('gomoku type: {}'.format(gt))
#             print(actions)
step = 0
while not board.is_over:
    step += 1
    if step % 2 == 1:
        start = time.time()
        action = board.vct(12, 100)
        print('vct time: {:.4f}'.format(time.time() - start))
    else:
        print(board)
        print('player: {}'.format(board.player))
        for is_player in [True, False]:
            print('is player: {}'.format(is_player))
            for gt in range(1, 6):
                actions = board.get_actions(is_player, gt)
                if len(actions) != 0:
                    print('gomoku type: {}'.format(gt))
                    print(actions)
        # input('any input to continue')
        action = tuple([int(x)-1 for x in input('action:\n').split()])
    board.move(action)
    print('move: ({} {})'.format(action[0]+1, action[1]+1))
