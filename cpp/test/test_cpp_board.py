import AlphaGomoku.cpp.board as cpp_board

# board = cpp_board.Board(cpp_board.IntVector([112, 97, 127]))

record = [
    (7, 7), (6, 7), (6, 8), (7, 8), (6, 6), (6, 5),
    (5, 6), (4, 6), (5, 5), (4, 4), (7, 6), (5, 9),
    (8, 6), (9, 6), (8, 7), (8, 5)
]

history = [r*15+c for r, c in record]
board = cpp_board.Board(cpp_board.IntVector(history))

print(board.get_positions(False, 6))
print(cpp_board.vct(board, 100, 100))
