print('start')
try:
	print('start try')
	a = 1 / 0
	print('end try')
except ZeroDivisionError:
	print('except')
finally:
	print('start finally')
	print('end finally')
print('middle')
try:
	print('start try')
	a = str(1) + 1
	print('end try')
except ZeroDivisionError:
	print('except')
finally:
	print('start finally')
	print('end finally')
print('end')