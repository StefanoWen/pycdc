def outout():
	gg = 0
	gg2 = 0

	def no():
		nonlocal gg
		nonlocal gg2
		print('nice')
		print(gg)
		print(gg2)

	def no_default(a, b, c, d):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def var(a, b, c, *args):
		nonlocal gg
		print(a, b, c)
		print(args)
		print(gg)

	def kw(a, b, c, **kwargs):
		nonlocal gg
		print(a, b, c)
		print(kwargs)
		print(gg)

	def var_kw(a, b, c, *args, **kwargs):
		nonlocal gg
		print(a, b, c)
		print(args)
		print(kwargs)
		print(gg)

	def one_default(a, b, c, d = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def one_default_kw(a, b, c, d = 1, **kwargs):
		nonlocal gg
		print(a, b, c, d)
		print(kwargs)
		print(gg)

	def one_default_var(a, b, c, d = 1, *args):
		nonlocal gg
		print(a, b, c, d)
		print(args)
		print(gg)

	def one_default_var_kw(a, b, c, d = 1, *args, **kwargs):
		nonlocal gg
		print(a, b, c, d)
		print(args)
		print(kwargs)
		print(gg)

	def two_default(a, b, c = 1, d = 2):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def three_default(a, b = 1, c = 2, d = 3):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def four_default(a = 1, b = 2, c = 3, d = 4):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def one_annot(a, b, c, d: int):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def one_annot_one_default(a, b, c, d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def two_annot(a, b, c: str, d: int):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def two_annot_two_default(a, b, c: str = '1', d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def three_annot_two_default(a: int, b, c: str = '1', d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def three_annot_three_default(a: int, b = '1', c: str = '2', d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def no1():
		nonlocal gg
		nonlocal gg2
		print('nice')
		print(gg)
		print(gg2)

	def no_default1(a, b, *, c, d):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def var1(a, b, c, *args):
		nonlocal gg
		print(a, b, c)
		print(args)
		print(gg)

	def kw1(a, b, *, c, **kwargs):
		nonlocal gg
		print(a, b, c)
		print(kwargs)
		print(gg)

	def var_kw1(a, b, c, *args, **kwargs):
		nonlocal gg
		print(a, b, c)
		print(args)
		print(kwargs)
		print(gg)

	def one_default1(a, *, b, c, d = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def one_default_kw1(a, b, *, c, d = 1, **kwargs):
		nonlocal gg
		print(a, b, c, d)
		print(kwargs)
		print(gg)

	def one_default_var1(a, b, c, d = 1, *args):
		nonlocal gg
		print(a, b, c, d)
		print(args)
		print(gg)

	def one_default_var_kw1(a, b, c, d = 1, *args, **kwargs):
		nonlocal gg
		print(a, b, c, d)
		print(args)
		print(kwargs)
		print(gg)

	def two_default1(a, b, *, c = 1, d = 2):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def three_default1(a, *, b = 1, c = 2, d = 3):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def four_default1(*, a = 1, b = 2, c = 3, d = 4):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def one_annot1(a, b, *, c, d: int):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def one_annot_one_default1(a, b, *, c, d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def two_annot1(a, b, c: str, *, d: int):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def two_annot_two_default1(a, *, b, c: str = '1', d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def three_annot_two_default1(a: int, b, *, c: str = '1', d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)

	def three_annot_three_default1(a: int, *, b = '1', c: str = '2', d: int = 1):
		nonlocal gg
		print(a, b, c, d)
		print(gg)