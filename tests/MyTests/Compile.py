import os
from pathlib import Path
import glob
import sys
import re

class Compile:
	def __init__(self, input_dir, compiled_dir, ver='', file_basename_exp=''):
		self.source_file_exp = file_basename_exp + '.py'
		self.input_dir = Path(input_dir)
		self.compiled_dir = Path(compiled_dir)
		self.versions = [ver] if ver else MyUtil.python_versions

	def py_compile_version(self, python_path, source_file):
		cmd_in = '"{}" -m py_compile "{}"'.format(str(python_path), source_file)
		MyUtil.run_cmd(cmd_in)
	
	def compile_all(self):
		MyUtil.print_start(False, self.source_file_exp)
		MyUtil.create_dir_if_not_exists(self.compiled_dir)
		python_path = MyUtil.get_python_versions_dir() / ('Python%s' % self.versions[0]) / 'python.exe'
		pycache_dir = self.input_dir / '__pycache__'
		
		for py_ver in self.versions:
			print('Compiling Version [ %s ]... ' % py_ver, end='')
			sys.stdout.flush()
			
			python_path =  python_path.parent.parent / ('Python%s' % py_ver) / 'python.exe'
			for source_file in glob.glob(str(self.input_dir  / self.source_file_exp)):
				source_file = Path(source_file)
				MyUtil.start_print_filename(source_file.name)
				
				pyc_file = self.compiled_dir / (source_file.stem + '_%s' % py_ver + '.pyc')
				
				if pyc_file.is_file():
					MyUtil.end_print_filename(len(source_file.name))
					continue
				
				self.py_compile_version(python_path, source_file)
				
				# move pyc file
				pyc_out_path = self.input_dir / (source_file.stem + '.pyc')
				if not pyc_out_path.is_file():
					pyc_out_path = pycache_dir / ('%s.cpython-%s.pyc' % (source_file.stem, py_ver))
				pyc_out_path.replace(pyc_file)
				
				MyUtil.end_print_filename(len(source_file.name))
			print('Done.')
		print('')
		if os.path.isdir(pycache_dir):
			os.rmdir(pycache_dir)


def main():
	ver = ''
	file_basename_exp = '*'
	input_dir = ''
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg.startswith('input_dir='):
				input_dir = arg.split('=', 1)[1]
			elif arg.startswith('exp='):
				file_basename_exp = arg.split('=', 1)[1]
				file_basename_exp = re.sub('[^*a-zA-Z0-9_-]', '', file_basename_exp)
			elif arg.isdigit():
				ver = arg
	if not input_dir:
		print('Need to give path to input dir (input_dir=/path/to/input_dir)')
		sys.exit(1)
	compile = Compile(input_dir=input_dir, compiled_dir='./compiled/', ver=ver, file_basename_exp=file_basename_exp)
	compile.compile_all()


if __name__ == '__main__':
	import MyUtil
	main()
else:
	from . import MyUtil



