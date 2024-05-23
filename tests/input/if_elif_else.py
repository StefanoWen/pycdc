def func1():
	a = 1
	if a < 2:
		print('<2')
		if a < 1:
			print('<1')
	else:
		print('else')
	print('end')

def func2():
	a = 1
	if a < 2:
		print('<2')
		if a < 1:
			print('<1')
		else:
			print('else')
	print('end')

def func3():
	a = 1
	if a < 2:
		print('<2')
		if a < 1:
			print('<1')
	print('end')

def func4():
	a = 1
	if a < 2 and a < 1:
		print('<2')
	else:
		print('else')
	print('end')

def func5():
	a = 1
	if not (a < 2) and a < 1:
		print('<2')
	else:
		print('else')
	print('end')

def func6():
	a = 1
	if a < 2 and not (a < 1):
		print('<2')
	else:
		print('else')
	print('end')

def func7():
	a = 1
	if a < 2 or a < 1:
		print('<2')
	else:
		print('else')
	print('end')

def func8():
	a = 1
	if not (a < 2) or a < 1:
		print('<2')
	else:
		print('else')
	print('end')

def func9():
	a = 1
	if a < 2 or not (a < 1):
		print('<2')
	else:
		print('else')
	print('end')

def func10():
	a = 1
	if a > 1:
		print('if')
	elif a == 1:
		print('elif')
	else:
		print('else')
	print('end')
a = 1
if a > 2:
	print('a>2')
if a < 2:
	print('a<2')
else:
	print('else')
if a == 2:
	print('a==2')
elif a > 2:
	print('a>2')
else:
	print('else')
if a == 1 and a == 2:
	print('a==1 and a==2')
if a == 1 and a == 2:
	print('a==1 and a==2')
else:
	print('else')
if a == 1:
	print('a==1')
	if a == 2:
		print('a==2')
if a == 1:
	print('a==1')
	if a == 2:
		print('a==2')
else:
	print('else')
if a == 1:
	print('a==1')
	if a == 2:
		print('a==2')
	else:
		print('a!=2')
if a == 1:
	print('a==1')
	if a == 2:
		print('a==2')
	else:
		print('a!=2')
else:
	print('a!=1')
if not a == 1:
	print('not a==1')
if not a < 2 and a < 1:
	print('not a<2 and a<1')
else:
	print('else')
if a < 2 and not a < 1:
	print('a<2 and not a<1')
else:
	print('else')
if not (a == 1 and a == 2):
	print('not (a==1 and a==2)')
if a == 1 or a == 2:
	print('a==1 or a==2')
if a == 1 or a == 2:
	print('a==1 or a==2')
else:
	print('else')
if not (a == 1 or a == 2):
	print('not (a==1 or a==2)')
if not a == 1 or a == 2:
	print('not a==1 or a==2')
if not a == 1 or a == 2:
	print('not a==1 or a==2')
else:
	print('else')
if a == 1 or not a == 2:
	print('a==1 or not a==2')
if a == 1 or not a == 2:
	print('a==1 or not a==2')
else:
	print('else')
if (a == 1 and a == 2) or a == 3:
	print('(a==1 and a==2) or a==3')
if (a == 1 or a == 2) and a == 3:
	print('(a==1 or a==2) and a==3')
if (a == 1 or a == 2) and not a == 3:
	print('(a==1 or a==2) and not a==3')
a = 1
b = 2
c = a and b
c = not a and b
c = a and not b
c = not(a and b)
c = a or b
c = not a or b
c = a or not b
c = not(a or b)
