from AlphaGomoku.mcts.base import MCTSBoard

board = MCTSBoard()
for action in [(7, 7), (7, 8), (9, 7)]:
    print([action for action in board.get_legal_actions() if board.check_bound(action)])
    board.move(action)
