import os
from pathlib import Path
import glob
import sys
import re

class Decompile:
	def __init__(self, tests_out_versions_dir, old=True, ver='', file_basename_exp=''):
		self.pyc_file_exp = file_basename_exp + '.pyc'
		self.tests_out_versions_dir = Path(tests_out_versions_dir)
		if old:
			self.pycdc_path = MyUtil.run_cmd('where pycdc', True).split('\n')[0]
		else:
			self.pycdc_path = Path('../Release/pycdc.exe').resolve(strict=1)
		self.versions = [ver] if ver else MyUtil.python_versions
	
	def get_pycdc_output(self, pyc_path):
		cmd_in = '"{}" "{}"'.format(self.pycdc_path, pyc_path)
		return MyUtil.run_cmd(cmd_in, True, True)
	
	def decompile_one(self, pyc_file, source_file):
		cmd_out, cmd_err, retcode = self.get_pycdc_output(pyc_file)
		if retcode:
			decompiled_py_content = ('#ERROR0\n' +
									'Unexpected return code: ' +
									str(hex(retcode)) + '\n' +
									cmd_err.strip())
		elif cmd_err:
			decompiled_py_content = ('#ERROR1\n' + 
									cmd_err.strip())
		else:
			decompiled_py_content = cmd_out
		with open(source_file, 'w') as f:
			f.write(decompiled_py_content)
	
	def decompile_all(self):
		MyUtil.print_start(True, self.pyc_file_exp)
		
		for py_ver in self.versions:
			print('Decompiling Version [ %s ]... ' % py_ver, end='')
			sys.stdout.flush()
			
			version_dir = self.tests_out_versions_dir / py_ver
			for pyc_file in glob.glob(str(version_dir / self.pyc_file_exp)):
				pyc_file = Path(pyc_file)
				MyUtil.start_print_filename(pyc_file.name)
				
				source_file = version_dir / (pyc_file.stem + '.py')
				
				# decompile
				self.decompile_one(pyc_file, source_file)
				
				MyUtil.end_print_filename(len(pyc_file.name))
			print('Done.')
		print('')


def main():
	old = False
	ver = ''
	file_basename_exp = '*'
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg == 'old':
				old = True
			elif arg.startswith('exp='):
				file_basename_exp = arg.split('=', 1)[1]
				file_basename_exp = re.sub('[^*a-zA-Z0-9_-]', '', file_basename_exp)
			elif arg.isdigit():
				ver = arg
	decompile = Decompile(tests_out_versions_dir='./tests_out_versions/', old=old, ver=ver, file_basename_exp=file_basename_exp)
	decompile.decompile_all()


if __name__ == '__main__':
	import MyUtil
	main()
else:
	from . import MyUtil














