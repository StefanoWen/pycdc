import os
from pathlib import Path
import glob
import sys
import re
import time
import re

class Compile:
	def __init__(self, input_files_dir, tests_out_versions_dir, ver='', file_basename_exp=''):
		self.source_file_exp = file_basename_exp + '.py'
		self.input_files_dir = Path(input_files_dir)
		self.tests_out_versions_dir = Path(tests_out_versions_dir)
		self.versions = [ver] if ver else MyUtil.python_versions
		
		self.python_path = (MyUtil.get_python_versions_dir() / ('Python%s' % self.versions[0]) / 'python.exe')
		self.pycache_dir = self.input_files_dir / '__pycache__'

	def py_compile_version(self, source_file):
		cmd_in = '"{}" -m py_compile "{}"'.format(str(self.python_path), source_file)
		MyUtil.run_cmd(cmd_in)
	
	def compile_one(self, source_file, pyc_file, py_ver):
		if pyc_file.is_file():
			return
		
		self.py_compile_version(source_file)
		
		# move pyc file
		pyc_out_path = self.input_files_dir / (source_file.stem + '.pyc')
		if not pyc_out_path.is_file():
			pyc_out_path = self.pycache_dir / ('%s.cpython-%s.pyc' % (source_file.stem, py_ver))
		pyc_out_path.replace(pyc_file)
	
	def compile_all(self):
		MyUtil.print_start(False, self.source_file_exp)
		MyUtil.create_dir_if_not_exists(self.tests_out_versions_dir)
		
		for py_ver in self.versions:
			print('Compiling Version [ %s ]... ' % py_ver, end='')
			sys.stdout.flush()
			
			self.python_path =  self.python_path.parent.parent / ('Python%s' % py_ver) / 'python.exe'
			version_dir = self.tests_out_versions_dir / py_ver
			MyUtil.create_dir_if_not_exists(version_dir)
			
			input_files_dir_exp = str(self.input_files_dir  / self.source_file_exp)
			for source_file in glob.glob(input_files_dir_exp):
				source_file = Path(source_file)
				MyUtil.start_print_filename(source_file.name)
				
				pyc_file = version_dir / (source_file.stem + '.pyc')
				
				# compile
				self.compile_one(source_file, pyc_file, py_ver)
				
				MyUtil.end_print_filename(len(source_file.name))
			print('Done.')
		print('')
		if os.path.isdir(self.pycache_dir):
			os.rmdir(self.pycache_dir)


def main():
	ver = ''
	file_basename_exp = '*'
	input_files_dir = ''
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg.startswith('input_dir='):
				input_files_dir = arg.split('=', 1)[1]
			elif arg.startswith('exp='):
				file_basename_exp = arg.split('=', 1)[1]
				file_basename_exp = re.sub('[^*a-zA-Z0-9_-]', '', file_basename_exp)
			elif arg.isdigit():
				ver = arg
	if not input_files_dir:
		print('Need to give path to input dir (input_dir=/path/to/input_dir)')
		sys.exit(1)
	compile = Compile(input_files_dir=input_files_dir, tests_out_versions_dir='./tests_out_versions/', ver=ver, file_basename_exp=file_basename_exp)
	compile.compile_all()


if __name__ == '__main__':
	import MyUtil
	main()
else:
	from . import MyUtil



