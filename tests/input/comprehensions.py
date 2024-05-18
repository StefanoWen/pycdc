def list_comprehension():
    l = [x for x in 'hello']
    l2 = [x for y in ['hello', 'world'] for x in y]
    l3 = [(i, i ** 2) for i in range(4) if i % 2 == 0]
    l4 = [i * j for i in range(4) if i == 2 for j in range(7) if i + (i % 2) == 0]

def set_comprehension():
    s = {x for x in 'hello'}
    s2 = {x for y in ['hello', 'world'] for x in y}
    s3 = {(i, i ** 2) for i in range(4) if i % 2 == 0}
    s4 = {i * j for i in range(4) if i == 2 for j in range(7) if i + (i % 2) == 0}

def dict_comprehension():
    d = {x:x for x in 'hello'}
    d2 = {x:y for y in ['hello', 'world'] for x in y}
    d3 = {(i, i ** 2):i for i in range(4) if i % 2 == 0}
    d4 = {i * j:j for i in range(4) if i == 2 for j in range(7) if i + (i % 2) == 0}
