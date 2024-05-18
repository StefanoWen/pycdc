def try_except():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')
	
	print('end')

def try_except_else():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')
	else:
		print('start else')
		print('end else')
	
	print('end')

def try_except_else_finally():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')
	else:
		print('start else')
		print('end else')
	finally:
		print('start finally')
		print('end finally')
	
	print('end')

def try_except_else_no_ending():
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')
	else:
		print('start else')
		print('end else')

def try_except_except():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except TypeError:
		print('start except1')
		print('end except1')
	except ZeroDivisionError:
		print('start except2')
		print('end except2')
	
	print('end')

def try_except_except_else():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except TypeError:
		print('start except1')
		print('end except1')
	except ZeroDivisionError:
		print('start except2')
		print('end except2')
	else:
		print('start else')
		print('end else')
	
	print('end')

def try_except_except_else_finally():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except TypeError:
		print('start except1')
		print('end except1')
	except ZeroDivisionError:
		print('start except2')
		print('end except2')
	else:
		print('start else')
		print('end else')
	finally:
		print('start finally')
		print('end finally')
	
	print('end')

def try_except_except_finally():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except TypeError:
		print('start except1')
		print('end except1')
	except ZeroDivisionError:
		print('start except2')
		print('end except2')
	finally:
		print('start finally')
		print('end finally')
	
	print('end')

def try_except_finally():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError:
		print('start except')
		print('end except')
	finally:
		print('start finally')
		print('end finally')
	
	print('end')

def try_except_match():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError:
		print('start except')
		print('end except')
	
	print('end')

def try_except_match_as():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError as e:
		print('start except')
		print('end except')
	
	print('end')

def try_finally():
	print('start')
	
	try:
		print('start try')
		print('end try')
	finally:
		print('start finally')
		print('end finally')
	
	print('end')
