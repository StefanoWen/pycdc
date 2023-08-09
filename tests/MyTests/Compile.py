import os
from pathlib import Path
import glob
import shutil
import sys
import MyUtil

class Compile:
	def __init__(self, skip=False, ver=''):
		self.python_versions = MyUtil.python_versions
		self.skip = skip
		self.ver = ''
		if ver:
			self.ver = ver

	def py_compile_verion(self, python_path, python_source_path):
		cmd_in = '"{}" -m py_compile "{}"'.format(python_path, python_source_path)
		MyUtil.run_cmd(cmd_in)

	def check_versions_dir(self):
		if not os.path.isdir('./versions/'):
			os.makedirs('./versions/')
	
	def print_start(self):
		print('======================')
		print('Starting compile...', end='')
		if self.skip:
			print('\n(Skipping compilation if already exists...)')
		else:
			print('\n(Add param "skip" to skip already exists)')
		print('======================')
	
	def compile_all(self):
		self.print_start()
		python_versions_dir = Path(MyUtil.get_python_path()).parent.parent
		self.check_versions_dir()
		versions = self.python_versions
		if self.ver:
			versions = [self.ver]
		for py_ver in versions:
			print('Compiling Version [ %s ]... ' % py_ver, end='')
			sys.stdout.flush()
			python_path = python_versions_dir.joinpath('Python%s' % py_ver).joinpath('python.exe').absolute()
			py_compiled_ver_dir = Path('./versions') / py_ver
			if not py_compiled_ver_dir.is_dir():
				os.makedirs(py_compiled_ver_dir.absolute())
			for python_source_file in glob.glob('./src/*.py'):
				python_source_file = Path(python_source_file)
				pyc_out_new_path = py_compiled_ver_dir.joinpath(python_source_file.stem + '.pyc')
				if pyc_out_new_path.is_file() and self.skip:
					continue
				print(python_source_file.name, end='')
				sys.stdout.flush()
				printed_filename_len = len(python_source_file.name)
				self.py_compile_verion(python_path, python_source_file.absolute())
				pyc_out_path = python_source_file.parent.joinpath(python_source_file.stem + '.pyc')
				if not pyc_out_path.is_file():
					pyc_out_path = python_source_file.parent.joinpath('__pycache__').joinpath('%s.cpython-%s.pyc' % (python_source_file.stem, py_ver))
				pyc_out_path.replace(pyc_out_new_path.absolute())
				to_erase_str = '\b'*printed_filename_len
				to_erase_str += ' '*printed_filename_len
				to_erase_str += '\b'*printed_filename_len
				print(to_erase_str , end='')
			print('Done.')
		print('')
		if os.path.isdir('./src/__pycache__/'):
			os.rmdir('./src/__pycache__/')


def main():
	skip = False
	ver = ''
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg == 'skip':
				skip = True
			elif arg.isdigit():
				ver = arg
	compile = Compile(skip=skip, ver=ver)
	compile.compile_all()


if __name__ == '__main__':
	main()




