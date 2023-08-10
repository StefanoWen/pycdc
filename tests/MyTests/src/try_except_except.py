print('start')

try:
	print('start try')
	a = 1 / 0
	print('end try')
except TypeError:
	print('except1')
except ZeroDivisionError:
	print('except2')

print('middle')

try:
	print('start try')
	a = str(1) + 1
	print('end try')
except TypeError:
	print('except1')
except ZeroDivisionError:
	print('except2')

print('end')