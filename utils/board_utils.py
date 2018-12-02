import numpy as np
from ..global_constants import *

def rotate_action(action, degree):
    k = (degree if degree % 90 != 0 else degree // 90) % 4

    if k == 0:
        return action

    row, col = action
    if k == 1:
        return (col, BOARD_SIZE - 1 - row)
    elif k == 2:
        return (BOARD_SIZE - 1 - row, BOARD_SIZE - 1 - col)
    else:
        return (BOARD_SIZE - 1 - col, row)

def rotate_tensor(tensor, degree, axes=(0, 1)):
    k = (degree if degree % 90 != 0 else degree // 90) % 4

    if k == 0:
        return tensor

    first, second = axes
    sign = 1 if first > second else -1
    if k == 3:
        k = 1
        sign *= -1
    return np.rot90(tensor, sign * k, axes)

def flip_action(action, pattern):
    if type(pattern) == str:
        if pattern == 'row':
            pattern = 0
        elif pattern == 'col':
            pattern = 1
        elif pattern == 'diag':
            pattern = 2
        elif pattern == 'back_diag':
            pattern = 3

    row, col = action
    if pattern == 0:
        return (BOARD_SIZE - 1 - row, col)
    elif pattern == 1:
        return (row, BOARD_SIZE - 1 - col)
    elif pattern == 2:
        return (col, row)
    elif pattern == 3:
        return (BOARD_SIZE - 1 - col, BOARD_SIZE - 1 - row)
    else:
        raise Exception('Unknown pattern: {}'.format(pattern))

def flip_tensor(tensor, pattern, axes=(0, 1)):
    if type(pattern) == str:
        if pattern == 'row':
            pattern = 0
        elif pattern == 'col':
            pattern = 1
        elif pattern == 'diag':
            pattern = 2
        elif pattern == 'back_diag':
            pattern = 3

    first, second = axes
    shape = np.shape(tensor)
    if pattern == 0:
        indices = [slice(None)] * len(shape)
        first_dimension = shape[first]
        indices[first] = slice(first_dimension-1, None, -1)
        return tensor[indices]
    elif pattern == 1:
        indices = [slice(None)] * len(shape)
        second_dimension = shape[second]
        indices[second] = slice(second_dimension-1, None, -1)
        return tensor[indices]
    elif pattern == 2:
        shuffle_axes = list(range(len(shape)))
        shuffle_axes[first], shuffle_axes[second] = shuffle_axes[second], shuffle_axes[first]
        return np.transpose(tensor, shuffle_axes)
    elif pattern == 3:
        temp_tensor = rotate_tensor(tensor, 3, first, second)
        shuffle_axes = list(range(len(shape)))
        shuffle_axes[first], shuffle_axes[second] = shuffle_axes[second], shuffle_axes[first]
        temp_tensor = np.transpose(temp_tensor, shuffle_axes)
        return rotate_tensor(temp_tensor, 1, first, second)
    else:
        raise Exception('Unknown pattern: {}'.format(pattern))

rotate0_action = lambda action: rotate_action(action, 0)
rotate90_action = lambda action: rotate_action(action, 1)
rotate180_action = lambda action: rotate_action(action, 2)
rotate270_action = lambda action: rotate_action(action, 3)

rotate0_tensor = lambda tensor, axes=(0, 1): rotate_tensor(tensor, 0, axes)
rotate90_tensor = lambda tensor, axes=(0, 1): rotate_tensor(tensor, 1, axes)
rotate180_tensor = lambda tensor, axes=(0, 1): rotate_tensor(tensor, 2, axes)
rotate270_tensor = lambda tensor, axes=(0, 1): rotate_tensor(tensor, 3, axes)

flip_row_action = lambda action: flip_action(action, 0)
flip_col_action = lambda action: flip_action(action, 1)
flip_diag_action = lambda action: flip_action(action, 2)
flip_back_diag_action = lambda action: flip_action(action, 3)

flip_row_tensor = lambda tensor, axes=(0, 1): flip_action(tensor, 0, axes)
flip_col_tensor = lambda tensor, axes=(0, 1): flip_action(tensor, 1, axes)
flip_diag_tensor = lambda tensor, axes=(0, 1): flip_action(tensor, 2, axes)
flip_back_diag_tensor = lambda tensor, axes=(0, 1): flip_action(tensor, 3, axes)

action_functions = [
    rotate0_action, rotate90_action,
    rotate180_action, rotate270_action,
    flip_row_action, flip_col_action,
    flip_diag_action, flip_back_diag_action
]

tensor_functions = [
    rotate0_tensor, rotate90_tensor,
    rotate180_tensor, rotate270_tensor,
    flip_row_tensor, flip_col_tensor,
    flip_diag_tensor, flip_back_diag_tensor
]

recover_action_functions = {
    rotate0_action: rotate0_action,
    rotate90_action: rotate270_action,
    rotate180_action: rotate180_action,
    rotate270_action: rotate90_action,
    flip_row_action: flip_row_action,
    flip_col_action: flip_col_action,
    flip_diag_action: flip_diag_action,
    flip_back_diag_action: flip_back_diag_action,

    rotate0_tensor: rotate0_action,
    rotate90_tensor: rotate270_action,
    rotate180_tensor: rotate180_action,
    rotate270_tensor: rotate90_action,
    flip_row_tensor: flip_row_action,
    flip_col_tensor: flip_col_action,
    flip_diag_tensor: flip_diag_action,
    flip_back_diag_tensor: flip_back_diag_action
}

recover_tensor_functions = {
    rotate0_action: rotate0_tensor,
    rotate90_action: rotate270_tensor,
    rotate180_action: rotate180_tensor,
    rotate270_action: rotate90_tensor,
    flip_row_action: flip_row_tensor,
    flip_col_action: flip_col_tensor,
    flip_diag_action: flip_diag_tensor,
    flip_back_diag_action: flip_back_diag_tensor,

    rotate0_tensor: rotate0_tensor,
    rotate90_tensor: rotate270_tensor,
    rotate180_tensor: rotate180_tensor,
    rotate270_tensor: rotate90_tensor,
    flip_row_tensor: flip_row_tensor,
    flip_col_tensor: flip_col_tensor,
    flip_diag_tensor: flip_diag_tensor,
    flip_back_diag_tensor: flip_back_diag_tensor
}
