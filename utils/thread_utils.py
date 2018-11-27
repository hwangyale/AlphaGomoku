import threading

RLOCK = threading.RLock()
CONDITION = threading.Condition(RLOCK)
