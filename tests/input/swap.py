def func1():
	a = 1
	b = 2
	c = 3
	a,b,c = c,a,b

def func2():
	arr = [1,2,3]
	arr[0],arr[1],arr[2] = arr[2],arr[0],arr[1]

def func3():
	a = 1
	b = 2
	a,b = b,a
	
def func4():
	a,b,c,d = 1,2,3,4
	a,b,c,d = d,c,b,a

def func5():
	a = '1'
	b = '2'
	c = '3'
	a,b,c = c,a,b
