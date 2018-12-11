import os
import numpy as np
from AlphaGomoku.process_data import process_history
from AlphaGomoku.utils.json_utils import json_dump_tuple

path = os.path.dirname(os.path.realpath(__file__))

def process_rec():
    history_container = []
    for root, dirs, files in os.walk(os.path.join(path, '__raw_data__')):
        for file in files:
            _, extension = os.path.splitext(file)
            if extension != '.rec':
                continue
            history = []
            with open(os.path.join(root, file), 'r') as f:
                for line in f.readlines():
                    if '\n' in line:
                        line = line.split('\n')[0]
                    if len(line.split(',')) != 3:
                        continue
                    row, col, t = line.split(',')
                    history.append((int(row), int(col), int(t)))
            history_container.append(history)
    return history_container

def main(index, max_number=100000):
    history_container = process_rec()
    tuples = process_history(history_container, True)
    indice = list(range(len(tuples)))
    np.random.shuffle(indice)
    tuples = [tuples[idx] for idx in indice[:max_number]]
    print('the number of tuples: {}'.format(len(tuples)))
    json_dump_tuple(tuples, os.path.join(path, 'pre', '{}.json'.format(index)))

if __name__ == '__main__':
    main(0)
