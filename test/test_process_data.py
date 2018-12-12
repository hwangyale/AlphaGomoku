from AlphaGomoku.process_data import *


tuples = [
    ([(0, 0), (1, 1), (2, 2)], (3, 3), 1.0),
    ([(0, 0), (2, 2)], (1, 1), 1.0),
    ([], (1, 1), 2.0)
]
nodes = tuples_to_tree(tuples)
print([(node.data_container, node.children) for node in nodes])
json_object = tree_to_json(nodes)
print(json_object)
nodes = json_to_tree(json_object)
print([(node.data_container, node.children) for node in nodes])
tuples = tree_to_tuples(nodes)
print(tuples)
