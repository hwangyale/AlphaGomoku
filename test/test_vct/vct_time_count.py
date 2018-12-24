import numpy as np
from AlphaGomoku.board import Board
from AlphaGomoku.data.process_raw_data import process_rec
from AlphaGomoku.process_data import check_legality
from AlphaGomoku.utils.json_utils import json_dump_tuple

def main(max_time):
    history_container = process_rec()
    indice = list(range(len(history_container)))
    np.random.shuffle(indice)
    history_container = [history_container[index] for index in indice[:512]]
    key_set = set()
    counter = {i: 0 for i in range(1, max_time+1)}
    for index, history in enumerate(history_container):
        print(index)
        if not check_legality(history):
            continue
        key = Board([(row, col) for row, col, _ in history]).zobristKey
        if key in key_set:
            continue
        board = Board()
        for row, col, t in history:
            action = (row, col)
            if t < 2 or t >= 300 or board.zobristKey in key_set:
                board.move(action)
                continue
            flag = False
            time = 0
            print('t:{}'.format(t))
            while time < max_time:
                time += 1
                print('time:{:.1f}'.format(0.1*time))
                if board.vct(16, time, False) is not None:
                    flag = True
                    counter[time] += 1
                    break
            key_set.add(board.zobristKey)
            if flag:
                break
            board.move(action)
        key_set.add(key)
        json_dump_tuple({str(key): value for key, value in counter.items()}, 'counter.json')
    print(counter)

if __name__ == '__main__':
    main(20)
