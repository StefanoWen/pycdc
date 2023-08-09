print('start')

try:
	print('start try')
	a = 1/0
	print('end try')
except:
	print('except')
else:
	print('else')
finally:
	print('start finally')
	print('end finally')
	
print('middle')

try:
	print('start try')
	a = 1
	print('end try')
except:
	print('except')
else:
	print('else')
finally:
	print('start finally')
	print('end finally')

print('end')