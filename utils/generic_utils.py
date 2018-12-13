__all__ = ['ProgBar', 'serialize_object', 'deserialize_object']
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

    def update(self, step=1, show='', endline=True):
        print('\r' + ' ' * (79) + '\r', end='')
        self.step += step
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
        if self.step > 0:
            left_time = (time.time() - self.start) * (self.steps - self.step) / (float(self.step))
        else:
            left_time = float('inf')
        if self.step < self.steps:
            print(' ETA: {:.0f}s'.format(left_time), end='')
            if show != '':
                print(' {}'.format(show), end='')
            sys.stdout.flush()
        else:
            total_time = time.time() - self.start
            time_per_step = (total_time / self.steps) * 1000
            if endline:
                end = '\n'
            else:
                end = ''
            print(' {:.0f}s {:.0f}ms/step'.format(total_time, time_per_step), end=end)


def serialize_object(instance, *args, **kwargs):
    return {
        'class_name': instance.__class__.__name__,
        'config': instance.get_config(*args, **kwargs)
    }

def deserialize_object(config, module_objects):
    class_name = config['class_name']
    if class_name not in module_objects:
        raise ValueError('Unknown class name: {}'.format(class_name))
    return module_objects[class_name].from_config(config['config'])

if __name__ == '__main__':
    import numpy as np
    rng = np.random
    progbar = ProgBar(1000)
    for _ in range(1000):
        progbar.update(rng.randint(2))
        time.sleep(0.01)
    print('finished!')
