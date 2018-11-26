__all__ = ['ProgBar']
import sys
import time


class ProgBar(object):
    def __init__(self, steps, length=20):
        self.steps = steps
        self.length = length
        self.step = 0
        self.start = time.time()

    def initialize(self):
        print('{:>6.2f}% |'.format(0) + ' ' * self.length + '|', end='')
        sys.stdout.flush()
        self.start = time.time()

    def update(self):
        print('\r' + ' ' * (79) + '\r', end='')
        self.step += 1
        percent = self.step / float(self.steps)
        print('{:>6.2f}% |'.format(percent * 100), end='')
        tmp_length = int(percent * self.length)
        print('=' * (tmp_length), end='')
        if tmp_length < self.length - 1:
            print('>', end='')
            print(' ' * (self.length - tmp_length - 1), end='')
        elif self.step < self.steps:
            print('>', end='')
        print('|', end='')
        left_time = (time.time() - self.start) * (self.steps - self.step) / float(self.step)
        if self.step < self.steps:
            print(' ETA: {:.0f}s'.format(left_time), end='')
            sys.stdout.flush()
        else:
            print(' ETA: {:.0f}s'.format(time.time() - self.start))


if __name__ == '__main__':
    progbar = ProgBar(1000)
    for _ in range(1000):
        progbar.update()
        time.sleep(0.01)
    print('finished!')
