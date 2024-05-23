from subprocess import Popen, PIPE
import os
from pathlib import Path
import glob
import shutil
import sys
import re
import time
import argparse
import difflib

WHITE_COLOR = 'white'
LRED_COLOR = 'light_red'
LGREEN_COLOR = 'light_green'
LYELLOW_COLOR = 'light_yellow'
LBLUE_COLOR = 'light_blue'
LCYAN_COLOR = 'light_cyan'
LMAGENTA_COLOR = 'light_magenta'

RED_COLOR = 'red'
GREEN_COLOR = 'green'
YELLOW_COLOR = 'yellow'
BLUE_COLOR = 'blue'
CYAN_COLOR = 'cyan'
MAGENTA_COLOR = 'magenta'

versions = {
		'3': ['0',
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',
		'10',
		'11',
		'12'
		]}

def run_cmd(cmd, with_output=False, with_err=False):
	proc = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
	cmd_stdout, cmd_stderr = proc.communicate()
	cmd_stdout = cmd_stdout.decode().replace('\r', '')
	cmd_stderr = cmd_stderr.decode().replace('\r', '')
	if with_err and with_output:
		return cmd_stdout, cmd_stderr, proc.returncode
	elif with_err:
		return cmd_stderr, proc.returncode
	elif with_output:
		return cmd_stdout
	else:
		return None

def eprint(*args, **kwargs):
	print(*args, file=sys.stderr, **kwargs)

def print_error_and_exit(error_msg, retcode=1):
	eprint('\nERROR:\n' + error_msg)
	sys.exit(retcode)

def print_info(indicate_char, info, head='', max_align='', color=WHITE_COLOR):
	if head and max_align:
		print('[{}] {} {}--> {}'.format(
			indicate_char*2, 
			head, 
			'-'*(max_align - len(head)),
			colored(info, color)))
	else:
		print('[{}] {}'.format(
			indicate_char*2, 
			colored(info, color)))

def start_print_same_line(filename):
	sys.stdout.write(filename)
	sys.stdout.flush()
	
def end_print_same_line(filename_len):
	to_erase_str = '\b'*filename_len
	to_erase_str += ' '*filename_len
	to_erase_str += '\b'*filename_len
	sys.stdout.write(to_erase_str)

def print_start(file_basename_exp):
	print('======================')
	print('Starting TESTS...')
	print('Files expression: < %s >' % colored(file_basename_exp, CYAN_COLOR))
	print('======================')

def create_dir_if_not_exists(dir_path):
	if not os.path.isdir(dir_path):
		os.makedirs(dir_path)

def compile(input_dir, compiled_dir, file_basename_exp):
	def get_python_versions_dir():
		return Path(run_cmd('where python', True).split('\n')[0]).parent.parent
	
	create_dir_if_not_exists(compiled_dir)
	python_versions_dir = get_python_versions_dir()
	pycache_dir = input_dir / '__pycache__'
	
	version_start_print = 'Compiling Version [ '
	start_print_same_line(version_start_print)
	for py_major_ver, py_minor_versions in versions.items():
		for py_minor_ver in py_minor_versions:
			version_print = '%s.%s ]... ' % (py_major_ver, py_minor_ver)
			start_print_same_line(version_print)
			
			py_ver = py_major_ver + py_minor_ver
			python_path =  python_versions_dir / ('Python%s' % py_ver) / 'python.exe'
			for source_file in glob.glob(str(input_dir  / (file_basename_exp + '.py'))):
				source_file = Path(source_file)
				if quiet_level < 1:
					start_print_same_line(source_file.name)
				
				pyc_file = compiled_dir / (source_file.stem + '.%s.%s' % (py_major_ver, py_minor_ver) + '.pyc')
				
				if pyc_file.is_file():
					end_print_same_line(len(source_file.name))
					continue
				
				cmd_in = '"{}" -m py_compile "{}"'.format(str(python_path), source_file)
				cmd_out, cmd_err, retcode = run_cmd(cmd_in, True, True)
				if retcode:
					end_print_same_line(len(source_file.name))
					print_error_and_exit(cmd_err, retcode)
				
				# move pyc file
				pyc_out_path = input_dir / (source_file.stem + '.pyc')
				if not pyc_out_path.is_file():
					pyc_out_path = pycache_dir / ('%s.cpython-%s.pyc' % (source_file.stem, py_ver))
				pyc_out_path.replace(pyc_file)
				
				if quiet_level < 1:
					end_print_same_line(len(source_file.name))
			end_print_same_line(len(version_print))
	end_print_same_line(len(version_start_print))
	print('Compilation Done.')
	print()
	if os.path.isdir(pycache_dir):
		os.rmdir(pycache_dir)

def decompile(compiled_dir, decompiled_dir, file_basename_exp, pycdc_path):
	def get_decompiled_output(pyc_file):
		cmd_in = '"{}" "{}"'.format(pycdc_path, pyc_file)
		cmd_out, cmd_err, retcode = run_cmd(cmd_in, True, True)
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
	
	create_dir_if_not_exists(decompiled_dir)
	
	version_start_print = 'Decompiling Version [ '
	start_print_same_line(version_start_print)
	for py_major_ver, py_minor_versions in versions.items():
		for py_minor_ver in py_minor_versions:
			version_print = '%s.%s ]... ' % (py_major_ver, py_minor_ver)
			start_print_same_line(version_print)
			
			for pyc_file in glob.glob(str(compiled_dir / (file_basename_exp + '.%s.%s' % (py_major_ver, py_minor_ver) + '.pyc'))):
				pyc_file = Path(pyc_file)
				if quiet_level < 1:
					start_print_same_line(pyc_file.name)
				
				decompiled_output = get_decompiled_output(pyc_file.resolve(strict=1))
				source_out_path = decompiled_dir / (pyc_file.stem + '.py')
				with open(source_out_path, 'w') as f:
					f.write(decompiled_output)
				
				if quiet_level < 1:
					end_print_same_line(len(pyc_file.name))
			end_print_same_line(len(version_print))
	end_print_same_line(len(version_start_print))
	print('Decompilation Done.')
	print()

def check_error0(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
	if decompiled_source_file_first_line.startswith('#ERROR0'):
		if quiet_level < 2:
			print_info('-', 'Failed (pycdc crashed in runtime)', source_file.name, max_align_need, LRED_COLOR)
			if debug:
				print('-----------------')
				print(decompiled_source_file_after_first_line)
				print('-----------------')
		return True
	return False

def check_error1(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
	if decompiled_source_file_first_line.startswith('#ERROR1'):
		if quiet_level < 2:
			print_info('-', 'Failed (Unsupported / Warning, etc.)', source_file.name, max_align_need, LRED_COLOR)
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
		source_file_contents = '\n'.join(list(filter(lambda x: x.strip() and not x[0] == '#', source_file_contents.split('\n'))))
		source_files_contents[source_file.name] = source_file_contents
	
	# remove first commment lines on decompiled source files
	decompiled_source_file_contents = '\n'.join(decompiled_source_file_contents.split('\n')[3:])
	# strip lines and remove empty lines
	decompiled_source_file_contents = '\n'.join(list(filter(lambda x: x.strip(), decompiled_source_file_contents.split('\n'))))
	
	if source_files_contents[source_file.name] != decompiled_source_file_contents:
		if quiet_level < 2:
			print_info('-', '%s %s' % (colored('Failed', LRED_COLOR), colored('(Different stdout)', LMAGENTA_COLOR)), source_file.name, max_align_need)
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

def print_summary(version_to_decompiled_count, input_files_count):
	versions_passed = 0
	versions_partially_passed = 0
	if quiet_level < 3:
		print('Each Version Summary: (%s tests)' % colored(str(input_files_count), LCYAN_COLOR))
		version_format = 'Version %s.%s'
		max_align_need = len(version_format % ('1', '11'))
		for (py_major_ver, py_minor_ver), decompiled_count in version_to_decompiled_count.items():
			if decompiled_count == input_files_count:
				indicate_char = '+'
				info_str = 'Passed'
				color = LGREEN_COLOR
				versions_passed += 1
			elif decompiled_count > 0:
				indicate_char = '*'
				info_str = 'Partially passed'
				color = YELLOW_COLOR
				versions_partially_passed += 1
			else:
				indicate_char = '-'
				info_str = 'Failed'
				color = LRED_COLOR
			print_info(indicate_char, info_str + ' (%d / %d)' % (decompiled_count, input_files_count), version_format % (py_major_ver, py_minor_ver), max_align_need, color=color)
		print()
	else:
		for decompiled_count in version_to_decompiled_count.values():
			if decompiled_count == input_files_count:
				versions_passed += 1
			elif decompiled_count > 0:
				versions_partially_passed += 1
	versions_count = len(version_to_decompiled_count)
	versions_failed = (versions_count - versions_passed - versions_partially_passed)
	print('Versions Summary: (%s versions)' % colored(str(versions_count), LCYAN_COLOR))
	if versions_passed == versions_count:
		print_info('+', 'PASSED ALL', color=LGREEN_COLOR)
	elif versions_partially_passed:
		to_print_list = [('+', 'Passed', versions_passed, LGREEN_COLOR),
			('*', 'Partially passed', versions_partially_passed, YELLOW_COLOR),
			('-', 'Failed', versions_failed, LRED_COLOR)]
		for indicate_char, status, count, color in to_print_list:
			if count:
				print_info(indicate_char, status + ' (%d / %d)' % (count, versions_count), color=color)
	else:
		print_info('-', 'FAILED ALL', color=LRED_COLOR)

def get_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('pycdc_path', help='e.g. "..\\Debug\\pycdc.exe"')
	parser.add_argument('-e', '--expression', default='*', help='Expression for the test files. (e.g. "exceptions" or "exceptions*")')
	parser.add_argument('-v', '--versions', nargs='*', default=[], help='Test specific version(s). (e.g. "310" or "39 310")')
	parser.add_argument('-q', '--quiet', type=int, default=0, help='Specify quiet level (0-3). (e.g. "-q 3" or "-q 0")')
	parser.add_argument('--debug', action='store_true', default=False, help='Print more information on fails/errors.')
	parser.add_argument('--no-color', action='store_true', default=False, help='Disable colors on the terminal.')
	args = parser.parse_args()
	for version in args.versions:
		if not version.isdigit():
			parser.error('Version must consist only of digits. (e.g. "39")')
		elif version[0] not in versions or version[1:] not in versions[version[0]]:
			parser.error(f'Version "{version}" not supported. Supported versions are: {versions}')
	if args.quiet < 0 or args.quiet > 3:
		parser.error('quiet level must be between 0-3 (inclusive).')
	if args.debug and args.quiet > 1:
		parser.error('debug can only be when quiet level <= 1')
	if args.pycdc_path:
		if not Path(args.pycdc_path).exists():
			parser.error(f'File "{args.pycdc_path}" not exists.')
	return args

def init_and_get_args():
	global versions
	global debug
	debug = False
	global quiet_level
	quiet_level = 0
	global with_color
	with_color = True
	file_basename_exp = '*'
	
	args = get_args()
	file_basename_exp = args.expression
	file_basename_exp = re.sub('[^*a-zA-Z0-9_-]', '', file_basename_exp)
	if args.versions:
		versions = {}
		for version in args.versions:
			if version[0] in versions:
				versions[version[0]].add(version[1:])
			else:
				versions[version[0]] = {version[1:]}
	with_color = not args.no_color
	quiet_level = args.quiet
	debug = args.debug
	
	if with_color:
		global colored
		from colorama import just_fix_windows_console
		from termcolor import colored as termcolor_colored
		def colored(str, color, *args, **kwargs):
			if with_color:
				return termcolor_colored(str, color, *args, **kwargs)
			else:
				return str
		just_fix_windows_console()
	
	return file_basename_exp, args.pycdc_path

def main():
	global max_align_need
	global source_files_contents
	
	file_basename_exp, pycdc_path = init_and_get_args()
	
	input_dir = Path('./input/')
	input_dir_exp = str(input_dir / (file_basename_exp + '.py'))
	input_files = glob.glob(input_dir_exp)
	if not input_files:
		print('No input files matched expression.')
		return
	
	print_start(file_basename_exp)
	tests_starting_time = time.time()
	max_align_need = len(max(input_files, key=len))
	source_files_contents = {}
	
	compiled_dir = Path('./compiled/')
	compile(input_dir, compiled_dir, file_basename_exp)
	decompiled_dir = Path('./decompiled/')
	decompile(compiled_dir, decompiled_dir, file_basename_exp, pycdc_path)
	
	version_to_decompiled_count = {}
	
	for py_major_ver, py_minor_versions in versions.items():
		for py_minor_ver in py_minor_versions:
			if quiet_level < 2:
				print('Testing Version [ %s.%s ]... ' % (py_major_ver, py_minor_ver))
				print('=====================')
			
			if (py_major_ver, py_minor_ver) not in version_to_decompiled_count:
				version_to_decompiled_count[(py_major_ver, py_minor_ver)] = 0
			
			for source_file in input_files:
				source_file = Path(source_file)
				decompiled_source_path = decompiled_dir / (source_file.stem + '.%s.%s' % (py_major_ver, py_minor_ver) + '.py')
				if check_failed(source_file, decompiled_source_path):
					continue
				if quiet_level < 2:
					print_info('+', 'Succeeded', source_file.name, max_align_need, LGREEN_COLOR)
				version_to_decompiled_count[(py_major_ver, py_minor_ver)] += 1
			if quiet_level < 2:
				print()
	if quiet_level < 2:
		print()
	print('Finished in %s seconds.' % colored('%.2f' % (time.time() - tests_starting_time), LCYAN_COLOR))
	print_summary(version_to_decompiled_count, len(input_files))


if __name__ == '__main__':
	main()
		

