import MyTests.Compile as Compile
import MyTests.Decompile as Decompile
import MyTests.MyUtil as MyUtil
from subprocess import Popen, PIPE
import os
from pathlib import Path
import glob
import shutil
import sys
import re
import time

debug = False
old = False
ver = ''
file_basename_exp = '*'
if len(sys.argv) > 1:
	for arg in sys.argv[1:]:
		if arg == 'debug':
			debug = True
		elif arg == 'old':
			old = True
		elif arg.startswith('exp='):
			file_basename_exp = arg.split('=', 1)[1]
			file_basename_exp = re.sub('[^*a-zA-Z0-9_-]', '', file_basename_exp)
		elif arg.isdigit():
			ver = arg

def print_start():
	print('======================')
	print('Starting TESTS...')
	print('======================')

def call_compile(input_dir, compiled_dir):
	compile = Compile.Compile(input_dir=input_dir.absolute(),
								compiled_dir=compiled_dir.absolute(),
								ver=ver,
								file_basename_exp=file_basename_exp)
	compile.compile_all()

def call_decompile(compiled_dir, decompiled_dir):
	decompile = Decompile.Decompile(input_dir=compiled_dir.absolute(),
									decompiled_dir=decompiled_dir.absolute(),
									old=old,
									ver=ver,
									file_basename_exp=file_basename_exp)
	decompile.decompile_all()

def check_error0(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
	if decompiled_source_file_first_line.startswith('#ERROR0'):
		MyUtil.print_info('-', 'Failed (pycdc crashed in runtime)', source_file.name, max_align_need)
		if debug:
			print('-----------------')
			print(decompiled_source_file_after_first_line)
			print('-----------------')
		return True
	return False

def check_error1(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
	if decompiled_source_file_first_line.startswith('#ERROR1'):
		MyUtil.print_info('-', 'Failed (Unsupported / Warning, etc.)', source_file.name, max_align_need)
		if debug:
			print('-----------------')
			print(decompiled_source_file_after_first_line)
			print('-----------------')
		return True
	return False

def check_stdout_failed(source_file, decompiled_source_file_contents):
	if source_file.name not in source_files_contents:
		with open(source_file) as f:
			source_file_contents = f.read()
		# replace tabs with spaces on source files
		source_file_contents = source_file_contents.replace('\t', ' '*4)
		# strip lines and remove empty lines
		source_file_contents = '\n'.join(list(filter(lambda x: x.strip(), source_file_contents.split('\n'))))
		source_files_contents[source_file.name] = source_file_contents
	
	# remove first commment lines on decompiled source files
	decompiled_source_file_contents = '\n'.join(decompiled_source_file_contents.split('\n')[3:])
	# strip lines and remove empty lines
	decompiled_source_file_contents = '\n'.join(list(filter(lambda x: x.strip(), decompiled_source_file_contents.split('\n'))))
	
	if source_files_contents[source_file.name] != decompiled_source_file_contents:
		MyUtil.print_info('-', 'Failed (Different stdout)', source_file.name, max_align_need)
		# putting a lot of prints so i can switch to .encode() easily
		if debug:
			print('-----------------')
			print(source_files_contents[source_file.name])
			print('=================')
			print(decompiled_source_file_contents)
			print('-----------------')
		return True
	return False

def check_failed(source_file, decompiled_source_path):
	with open(decompiled_source_path, 'r') as f:
		decompiled_source_content = f.read()
	decompiled_source_file_first_line, decompiled_source_file_after_first_line = decompiled_source_content.split('\n', 1)
	if check_error0(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
		return True
	elif check_error1(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
		return True
	elif check_stdout_failed(source_file, decompiled_source_content):
		return True
	return False

def print_summary(version_to_decompiled_count, input_files_count, first_version):
	print('Summary:')
	version_format = 'Version %s'
	max_align_need = len(version_format % first_version)
	for py_ver, decompiled_count in version_to_decompiled_count.items():
		if decompiled_count == input_files_count:
			indicate_char = '+'
			info_str = 'Passed'
		elif decompiled_count > 0:
			indicate_char = '*'
			info_str = 'Partially passed'
		else:
			indicate_char = '-'
			info_str = 'Failed'
		MyUtil.print_info(indicate_char, info_str + ' (%d / %d)' % (decompiled_count, input_files_count), version_format % py_ver, max_align_need)

def main():
	global max_align_need
	global source_files_contents
	
	input_dir = Path('./input/')
	input_dir_exp = str(input_dir / (file_basename_exp + '.py'))
	input_files = glob.glob(input_dir_exp)
	if not input_files:
		print('No input files matched expression.')
		return
	
	print_start()
	max_align_need = len(max(input_files, key=len))
	source_files_contents = {}
	
	compiled_dir = Path('./compiled/')
	call_compile(input_dir, compiled_dir)
	decompiled_dir = Path('./decompiled/')
	call_decompile(compiled_dir, decompiled_dir)
	
	python_versions_dir = MyUtil.get_python_versions_dir()
	if ver:
		versions = [ver]
	else:
		versions = MyUtil.python_versions
	version_to_decompiled_count = {}
	
	for py_ver in versions:
		print('Testing Version [ %s ]... ' % py_ver)
		print('=====================')
		if py_ver not in version_to_decompiled_count:
			version_to_decompiled_count[py_ver] = 0
		
		for source_file in input_files:
			source_file = Path(source_file)
			decompiled_source_path = decompiled_dir / (source_file.stem + '_%s' % py_ver + '.py')
			if check_failed(source_file, decompiled_source_path):
				continue
			MyUtil.print_info('+', 'Succeeded', source_file.name, max_align_need)
			version_to_decompiled_count[py_ver] += 1
		print()
	print('Done.')
	print_summary(version_to_decompiled_count, len(input_files), versions[0])


if __name__ == '__main__':
	main()
		

