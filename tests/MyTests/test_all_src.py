from subprocess import Popen, PIPE
import os
from pathlib import Path
import glob
import shutil
import sys
import Compile
import Decompile
import MyUtil

def get_number_of_source_files(dir_exp):
	num = 0
	for _ in glob.glob(dir_exp):
		num += 1
	return num

skip = False
debug = False
debug_d = False
old = False
ver = ''
if len(sys.argv) > 1:
	for arg in sys.argv[1:]:
		if arg == 'skip':
			skip = True
		elif arg == 'debug':
			debug = True
		elif arg == 'debug_d':
			debug_d = True
		elif arg == 'old':
			old = True
		elif arg.isdigit():
			ver = arg

if not skip:
	compile = Compile.Compile(skip=skip, ver=ver)
	compile.compile_all()
	decompile = Decompile.Decompile(skip=skip, debug=debug_d, old=old, ver=ver)
	decompile.decompile_all()


print('======================')
print('Starting TESTS...', end='')
print('======================')
summary_version_to_status = {}
python_source_files_dir = '.\\src\\*.py'
python_versions_dir = Path(MyUtil.get_python_path()).parent.parent
to_run_format = '"{}" "{}"'
max_align_need = len(MyUtil.get_longest_filename_in_dir_exp(python_source_files_dir))
versions = MyUtil.python_versions
if ver:
	versions = [ver]
for py_ver in versions:
	print('Testing Version [ %s ]... ' % py_ver)
	print('=====================')
	if py_ver not in summary_version_to_status:
		max_files_len = get_number_of_source_files(python_source_files_dir)
		summary_version_to_status[py_ver] = [0, max_files_len]
	python_path = python_versions_dir.joinpath('Python%s' % py_ver).joinpath('python.exe').absolute()
	for source_file in glob.glob('.\\src\\*.py'):
		source_file = Path(source_file)
		decompiled_source_file = Path('.\\versions\\%s\\%s' % (py_ver, source_file.name))
		with open(decompiled_source_file, 'r') as f:
			decompiled_source_file_first_line = f.readline()
			decompiled_source_content = f.read()
		if decompiled_source_file_first_line.startswith('#ERROR0'):
			if debug:
				MyUtil.print_info('-', 'Failed: (pycdc crashed in runtime)\n' + decompiled_source_content, source_file.name, max_align_need)
			else:
				MyUtil.print_info('-', 'Failed (pycdc crashed in runtime)', source_file.name, max_align_need)
			continue
		if decompiled_source_file_first_line.startswith('#ERROR1'):
			if debug:
				MyUtil.print_info('-', 'Failed: (Unsupported opcode / Warning, etc.)\n' + decompiled_source_content, source_file.name, max_align_need)
			else:
				MyUtil.print_info('-', 'Failed (Unsupported opcode / Warning, etc.)', source_file.name, max_align_need)
			continue
		to_run = to_run_format.format(python_path, source_file.absolute())
		src_out, src_out_err, _ = MyUtil.run_cmd(to_run, True, True)
		to_run = to_run_format.format(python_path, decompiled_source_file.absolute())
		decompiled_out,  decompiled_out_err, _ = MyUtil.run_cmd(to_run, True, True)
		if src_out != decompiled_out:
			if debug:
				to_print = src_out
				to_print += '-------------\n'
				to_print += decompiled_out
				MyUtil.print_info('-', 'Failed: (Different stdout)\n' + to_print, source_file.name, max_align_need)
			else:
				MyUtil.print_info('-', 'Failed (Different stdout)', source_file.name, max_align_need)
			continue
		src_out_err = src_out_err.split(':')[-1].strip()
		decompiled_out_err = decompiled_out_err.split(':')[-1].strip()
		if src_out_err != decompiled_out_err:
			MyUtil.print_info('-', 'Failed (Different stderr)', source_file.name, max_align_need)
			continue
		MyUtil.print_info('+', 'Succeeded', source_file.name, max_align_need)
		summary_version_to_status[py_ver][0] += 1
	print()
print('Done.')
version_format = 'Version %s'
max_align_need = len(version_format % versions[-1])
print('Summary:')
for py_ver, status in summary_version_to_status.items():
	if status[0] == status[1]:
		indicate_char = '+'
		info_str = 'Passed'
	elif status[0] > 0:
		indicate_char = '*'
		info_str = 'Partially passed'
	else:
		indicate_char = '-'
		info_str = 'Failed'
	MyUtil.print_info(indicate_char, info_str + ' (%d / %d)' % (status[0], status[1]), version_format % py_ver, max_align_need)


		

