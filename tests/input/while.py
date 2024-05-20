def f_while():
	a = 1
	while a < 2:
		print('inside while')
	print('outside while')

def f_while_with_ifs():
	a = 1
	while a < 2:
		print('inside while')
		if a == 1:
			print('continue')
			continue
		elif a == 2:
			print('break')
			break
		else:
			print('nothing')
	print('outside while')

# credit to kibernautas for the samples
def POP_JUMP_BACKWARD_IF_NOT_NONE():
	item = None
	while item is not None:
		print('start while')
		print('end while')

def POP_JUMP_BACKWARD_IF_NONE():
	item = None
	while item is None:
		print('start while')
		print('end while')

def POP_JUMP_BACKWARD_IF_FALSE():
	cond = False
	while not cond:
		print('start while')
		print('end while')

def POP_JUMP_BACKWARD_IF_TRUE():
	cond = True
	while cond:
		print('start while')
		print('end while')

