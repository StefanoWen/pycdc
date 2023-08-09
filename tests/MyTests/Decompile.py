import os
from pathlib import Path
import glob
import shutil
import sys
import MyUtil

class Decompile:
	def __init__(self, skip=False, debug=False, old=True, ver=''):
		self.python_versions = MyUtil.python_versions
		if old:
			self.pycdc_path = MyUtil.run_cmd('where pycdc', True).split('\n')[0]
		else:
			self.pycdc_path = Path('../../Release/pycdc.exe').absolute()
		self.skip = skip
		self.debug = debug
		self.ver = ''
		if ver:
			self.ver = ver
	
	def get_pycdc_output(self, pyc_path):
		cmd_in = '"{}" "{}"'.format(self.pycdc_path, pyc_path)
		return MyUtil.run_cmd(cmd_in, True, True)

	def check_versions_dir(self):
		if not os.path.isdir('./versions/'):
			os.makedirs('./versions/')
	
	def print_start(self):
		print('======================')
		print('Starting decompile...', end='')
		if self.skip:
			print('\n(Skipping decompilation if already exists...)')
		else:
			print('\n(Add param "skip" to skip already exists)')
		print('======================')
	
	def decompile_all(self):
		self.print_start()
		self.check_versions_dir()
		max_align_need = len(MyUtil.get_longest_filename_in_dir_exp('./versions/%s/*.pyc' % self.python_versions[0]))
		versions = self.python_versions
		if self.ver:
			versions = [self.ver]
		for py_ver in versions:
			if self.debug:
				print('Decompiling Version [ %s ]... ' % py_ver)
				print('=====================')
			else:
				print('Decompiling Version [ %s ]... ' % py_ver, end='')
				sys.stdout.flush()
			version_dir = Path('./versions') / py_ver
			if not version_dir.is_dir():
				os.makedirs(version_dir.absolute())
			for pyc_file in glob.glob('./versions/%s/*.pyc' % py_ver):
				pyc_file = Path(pyc_file)
				decompiled_py_path = version_dir.joinpath(pyc_file.stem + '.py')
				if decompiled_py_path.is_file() and self.skip:
					if self.debug:
						MyUtil.print_info('*', 'Exists-Skipped', pyc_file.name, max_align_need)
					continue
				if not self.debug:
					print(pyc_file.name, end='')
					sys.stdout.flush()
					printed_filename_len = len(pyc_file.name)
				cmd_out, cmd_err, retcode = self.get_pycdc_output(pyc_file.absolute())
				if retcode:
					cmd_out = '#ERROR0\n'
					cmd_out += 'pycdc crashed.\n'
					cmd_out += 'Unexpected return code: ' + str(hex(retcode)) + '\n'
					if self.debug:
						MyUtil.print_info('-', 'Failed (Unexpected return code)', pyc_file.name, max_align_need)
				elif cmd_err:
					cmd_out = '#ERROR1\n' + cmd_err.strip() + '\n'
					if self.debug:
						MyUtil.print_info('-', 'Failed', pyc_file.name, max_align_need)
				else:
					if self.debug:
						MyUtil.print_info('+', 'Succeeded', pyc_file.name, max_align_need)
				with open(decompiled_py_path.absolute(), 'w') as f:
						f.write(cmd_out)
				if not self.debug:
					to_erase_str = '\b'*printed_filename_len
					to_erase_str += ' '*printed_filename_len
					to_erase_str += '\b'*printed_filename_len
					print(to_erase_str , end='')
			if self.debug:
				print()
			else:
				print('Done.')
		print('')


def main():
	skip = False
	debug = False
	old = False
	ver = ''
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg == 'skip':
				skip = True
			elif arg == 'debug':
				debug = True
			elif arg == 'old':
				old = True
			elif arg.isdigit():
				ver = arg
	decompile = Decompile(skip=skip, debug=debug, old=old, ver=ver)
	decompile.decompile_all()


if __name__ == '__main__':
	main()














