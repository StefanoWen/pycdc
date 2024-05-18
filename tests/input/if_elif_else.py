a = 1
if a > 2:
	print('a>2')
if a < 2:
	print('a<2')
else:
	print('else')
if a == 2:
	print('a==2')
elif a % 2 == 0:
	print('a%2==0')
else:
	print('else')
if a == 1 and a == 2:
	print('a==1 and a==2')
if a == 1:
	print('a==1')
	if a == 2:
		print('a==2')
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
if not (a == 1 and a == 2):
	print('not (a==1 and a==2)')
if a == 1 or a == 2:
	print('a==1 or a==2')
if not (a == 1 or a == 2):
	print('not (a==1 or a==2)')
if not a == 1 or a == 2:
	print('not a==1 or a==2')
if a == 1 or not a == 2:
	print('a==1 or not a==2')
if (a == 1 and a == 2) or a == 3:
	print('(a==1 and a==2) or a==3')
if (a == 1 or a == 2) and a == 3:
	print('(a==1 or a==2) and a==3')
if (a == 1 or a == 2) and not a == 3:
	print('(a==1 or a==2) and not a==3')
