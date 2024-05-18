def f_for():
    a = 1
    for i in range(a):
        print('inside for')
    print('outside for')

def f_for_with_ifs():
    a = 1
    for i in range(a):
        print('inside for')
        if a == 1:
            print('continue')
            continue
        elif a == 2:
            print('break')
            break
        else:
            print('nothing')
    print('outside for')
