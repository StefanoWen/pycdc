print('start')
try:
	print('start try')
	a = 1 / 0
	print('end try')
finally:
	print('start finally')
	print('end finally')
print('end')