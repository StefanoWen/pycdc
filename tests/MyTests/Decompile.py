import os
from pathlib import Path
import glob
import sys
import re

class Decompile:
	def __init__(self, input_dir, decompiled_dir, old=True, ver='', file_basename_exp=''):
		self.file_basename_exp = file_basename_exp
		self.input_dir = Path(input_dir)
		self.decompiled_dir = Path(decompiled_dir)
		self.versions = [ver] if ver else MyUtil.python_versions
		
		if old:
			self.pycdc_path = MyUtil.run_cmd('where pycdc', True).split('\n')[0]
		else:
			self.pycdc_path = Path('../Release/pycdc.exe').resolve(strict=1)
	
	def get_pycdc_output(self, pyc_path):
		cmd_in = '"{}" "{}"'.format(self.pycdc_path, pyc_path)
		return MyUtil.run_cmd(cmd_in, True, True)
	
	def get_decompiled_output(self, pyc_file):
		cmd_out, cmd_err, retcode = self.get_pycdc_output(pyc_file.resolve(strict=1))
		if retcode:
			decompiled_output = ('#ERROR0\n' +
									'Unexpected return code: ' +
									str(hex(retcode)) + '\n' +
									cmd_err.strip())
		elif cmd_err:
			decompiled_output = ('#ERROR1\n' + 
									cmd_err.strip())
		else:
			decompiled_output = cmd_out
		return decompiled_output
	
	def decompile_all(self):
		MyUtil.print_start(True, self.file_basename_exp)
		MyUtil.create_dir_if_not_exists(self.decompiled_dir)
		
		for py_ver in self.versions:
			print('Decompiling Version [ %s ]... ' % py_ver, end='')
			sys.stdout.flush()
			
			for pyc_file in glob.glob(str(self.input_dir / (self.file_basename_exp + '_%s' % py_ver + '.pyc'))):
				pyc_file = Path(pyc_file)
				MyUtil.start_print_filename(pyc_file.name)
				
				decompiled_output = self.get_decompiled_output(pyc_file)
				source_out_path = self.decompiled_dir / (pyc_file.stem + '.py')
				with open(source_out_path, 'w') as f:
					f.write(decompiled_output)
				
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
	decompile = Decompile(decompiled_dir='./decompiled/', old=old, ver=ver, file_basename_exp=file_basename_exp)
	decompile.decompile_all()


if __name__ == '__main__':
	import MyUtil
	main()
else:
	from . import MyUtil














