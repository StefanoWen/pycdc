print('start')
try:
	print('start try')
	a = 1/0
	print('end try')
except ZeroDivisionError as e:
	print('except')
print('middle')
try:
	print('start try')
	a = 1/1
	print('end try')
except ZeroDivisionError as e:
	print('except')
print('end')