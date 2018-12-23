FLOATX = 'float32'
EPSILON = 1e-7
MAX_FLOAT = float(1e+8)
MAX_INT = int(1e+8)

BOARD_SIZE = 15
STONES = 225
EMPTY = 0
BLACK = 1
WHITE = 2
DRAW = 3

OPEN_FOUR = 1
FOUR = 2
OPEN_THREE = 3
THREE = 4
OPEN_TWO = 5
TWO = 6
BINARY_HEPLERS = [2**i for i in range(BOARD_SIZE**2)]

ATTACK_VCT_DEPTH = 14
ATTACK_VCT_TIME = 1
DEFEND_VCT_DEPTH = 6
DEFEND_VCT_TIME = 0.1

NETWORK_WIN_VALUE = 1.0
NETWORK_LOSS_VALUE = -1.0
NETWORK_DRAW_VALUE = 0.0

MCTS_BOUND = 3
MCTS_C_PUCT = 2.5
MCTS_TABLE_CONTAINER = 10000
MCTS_VIRTUAL_LOSS = 10
MCTS_VIRTUAL_VISIT = 10
MCTS_ROOT_VCT_DEPTH = 20
MCTS_ROOT_VCT_TIME = 10
MCTS_VCT_DEPTH = 12
MCTS_VCT_TIME = 0.3
MCTS_DEFEND_VCT_DEPTH = 8
MCTS_DEFEND_VCT_TIME = 0.01
MCTS_WIN_VALUE = 1.0
MCTS_LOSS_VALUE = -1.0
MCTS_DRAW_VALUE = 0.0
MCTS_TRAVERSAL_DEPTH = 6
MCTS_ROLLOUT_VCT_DEPTH = 10
MCTS_ROLLOUT_VCT_TIME = 0.1
MCTS_DIRICHLET_NOISE = 0.03
MCTS_DIRICHLET_WEIGHT = 0.25
MCTS_RL_SAMPLE_STEP = 8
