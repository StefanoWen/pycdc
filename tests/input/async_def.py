async def foobar():
	print('start foobar')
	print('end foobar')

async def barfoo():
	print('start barfoo')
	await foobar()
	print('end barfoo')
