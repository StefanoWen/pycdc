def f_while():
    a = 1
    while a < 2:
        print('inside while')
    print('outside while')

def f_while_with_ifs():
    a = 1
    while a < 2:
        print('inside while')
        if a == 1:
            print('continue')
            continue
        elif a == 2:
            print('break')
            break
        else:
            print('nothing')
    print('outside while')
