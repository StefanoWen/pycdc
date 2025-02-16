def try_except():
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')

def try_except_covering():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')
	
	print('end')

def try_except_else():
	try:
		print('start try')
		print('end try')
	except:
		print('start except')
		print('end except')
	else:
		print('start else')
		print('end else')

def try_except_else_covering():
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

def try_except_else_finally_covering():
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

def try_except_except():
	try:
		print('start try')
		print('end try')
	except TypeError:
		print('start except1')
		print('end except1')
	except ZeroDivisionError:
		print('start except2')
		print('end except2')

def try_except_except_covering():
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

def try_except_except_else_covering():
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

def try_except_except_else_finally_covering():
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

def try_except_except_finally_covering():
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
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError:
		print('start except')
		print('end except')
	finally:
		print('start finally')
		print('end finally')

def try_except_finally_covering():
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
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError:
		print('start except')
		print('end except')

def try_except_match_covering():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError:
		print('start except')
		print('end except')
	
	print('end')

def try_except_match_as():
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError as e:
		print('start except')
		print('end except')

def try_except_match_as_covering():
	print('start')
	
	try:
		print('start try')
		print('end try')
	except ZeroDivisionError as e:
		print('start except')
		print('end except')
	
	print('end')

def try_finally():
	try:
		print('start try')
		print('end try')
	finally:
		print('start finally')
		print('end finally')

def try_finally_covering():
	print('start')
	
	try:
		print('start try')
		print('end try')
	finally:
		print('start finally')
		print('end finally')
	
	print('end')
