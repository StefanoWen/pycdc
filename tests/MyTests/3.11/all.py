def foo(x):
    print(x)
foo(x=5)
l = [int(num.strip()) for num in '123 \n456 '.split('\n')]
a = 10
if a == 10:
    print('a is 10')
else:
    print('a is not 10')
try:
    raise Exception("I am an exception")
except Exception as e:
    print(e)
finally:
    print("The finally code")