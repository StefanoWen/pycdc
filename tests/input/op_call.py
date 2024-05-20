def func(*args, **kwargs):
	print('hello')
a = 1
func()
func(1)
func(a)
func(b = 1)
func(1, b = '2')
func(1, 2)
func(b = '1', c = '2')
func(1, 2, b = '3')
func(1, b = '2', c = '3')
func(1, 2, b = '3', c = '4')
