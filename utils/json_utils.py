__all__ = ['json_load_tuple', 'json_dump_tuple']
import json

class TupleEncoder(json.JSONEncoder):
    def encode(self, obj):
        def hint_tuples(item):
            if isinstance(item, tuple):
                return {'__tuple__': True, 'items': item}
            if isinstance(item, list):
                return [hint_tuples(e) for e in item]
            if isinstance(item, dict):
                return {key: hint_tuples(value) for key, value in item.items()}
            return item

        return super(TupleEncoder, self).encode(hint_tuples(obj))

def hinted_tuple_hook(obj):
    if '__tuple__' in obj:
        return tuple(obj['items'])
    else:
        return obj

def json_load_tuple(file_path):
    with open(file_path, 'r') as f:
        obj = json.load(f)
        return json.loads(obj, object_hook=hinted_tuple_hook)

encoder = TupleEncoder()
def json_dump_tuple(obj, file_path):
    with open(file_path, 'w') as f:
        json.dump(encoder.encode(obj), f)
