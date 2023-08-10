print('start')

try:
	print('start try')
	a = 1 / 0
	print('end try')
except:
	print('except')
	
print('middle')

try:
	print('start try')
	a = 1
	print('end try')
except:
	print('except')

print('end')