import matplotlib.pyplot as plt
from ..global_constants import *

def visualize(pairs, time, dx=-.5, dy=-.2):
    axes = plt.gca()
    axis = np.linspace(0, BOARD_SIZE+1, BOARD_SIZE+2)
    axes.patch.set_facecolor((255 / 256., 206 / 256., 118 / 256.))
    axes.grid(color='k', linestyle='-', linewidth=0.2, alpha=1.)
    for action, value in pairs:
        axes.text(action[0]+1+dx, action[1]+1+dy, '{:.2f}'.format(value))
    axes.set_xticks(axis)
    axes.set_yticks(axis)
    axes.set_xticklabels(['']+[str(i) for i in range(1, BOARD_SIZE+1)]+[''])
    axes.set_yticklabels(['']+[str(i) for i in range(1, BOARD_SIZE+1)]+[''])
    plt.show(block=False)
    if time is None:
        time = self.visualization_time
    plt.pause(time)
