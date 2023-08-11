from subprocess import Popen, PIPE
import os
from pathlib import Path
import glob
import shutil
import sys
import re
import time

WHITE_COLOR = 'white'
LRED_COLOR = 'light_red'
LGREEN_COLOR = 'light_green'
LYELLOW_COLOR = 'light_yellow'
LCYAN_COLOR = 'light_cyan'
RED_COLOR = LRED_COLOR
GREEN_COLOR = LGREEN_COLOR
YELLOW_COLOR = 'yellow'
CYAN_COLOR = 'cyan'

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

def my_colored(str, color, *args, **kwargs):
	if with_color:
		return colored(str, color, *args, **kwargs)
	else:
		return str

def print_info(indicate_char, info, head, max_align, color=WHITE_COLOR):
	print('[{}] {} {}--> {}'.format(
		indicate_char*2, 
		head, 
		'-'*(max_align - len(head)),
		my_colored(info, color)))
	

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
	if file_basename_exp != '*':
		print('Files expression: < %s >' % my_colored(file_basename_exp, CYAN_COLOR))
	print('======================')

def create_dir_if_not_exists(dir_path):
	if not os.path.isdir(dir_path):
		os.makedirs(dir_path)

def compile(input_dir, compiled_dir, versions, file_basename_exp):
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

def decompile(compiled_dir, decompiled_dir, versions, file_basename_exp, pycdc_path):
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
			print_info('-', 'Failed (pycdc crashed in runtime)', source_file.name, max_align_need, RED_COLOR)
			if debug:
				print('-----------------')
				print(decompiled_source_file_after_first_line)
				print('-----------------')
		return True
	return False

def check_error1(source_file, decompiled_source_file_first_line, decompiled_source_file_after_first_line):
	if decompiled_source_file_first_line.startswith('#ERROR1'):
		if quiet_level < 2:
			print_info('-', 'Failed (Unsupported / Warning, etc.)', source_file.name, max_align_need, RED_COLOR)
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
		if quiet_level < 2:
			print_info('-', 'Failed (Different stdout)', source_file.name, max_align_need, YELLOW_COLOR)
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
	print('Summary:')
	version_format = 'Version %s.%s'
	max_align_need = len(version_format % ('1', '11'))
	versions_succeeded = 0
	for (py_major_ver, py_minor_ver), decompiled_count in version_to_decompiled_count.items():
		if decompiled_count == input_files_count:
			indicate_char = '+'
			info_str = 'Passed'
			color = GREEN_COLOR
			versions_succeeded += 1
		elif decompiled_count > 0:
			indicate_char = '*'
			info_str = 'Partially passed'
			color = YELLOW_COLOR
		else:
			indicate_char = '-'
			info_str = 'Failed'
			color = RED_COLOR
		if quiet_level < 3:
			print_info(indicate_char, info_str + ' (%d out of %d tests)' % (decompiled_count, input_files_count), version_format % (py_major_ver, py_minor_ver), max_align_need, color=color)
	print()
	versions_count = len(version_to_decompiled_count)
	if versions_succeeded == versions_count:
		indicate_char = '+'
		info_str = 'PASSED ALL'
		color = GREEN_COLOR
	elif versions_succeeded > 0:
		indicate_char = '*'
		info_str = 'Partially passed' + ' (%d out of %d versions)' % (versions_succeeded, versions_count)
		color = YELLOW_COLOR
	else:
		indicate_char = '+'
		info_str = 'FAILED ALL'
		color = RED_COLOR
	print_info(indicate_char, info_str, 'Versions', 1, color=color)

def init_and_get_args():
	global debug
	debug = False
	global quiet_level
	quiet_level = 0
	global with_color
	with_color = True
	old_str = ''
	file_basename_exp = '*'
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
		#'11'
		]}
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg == 'debug':
				debug = True
			elif arg == 'old':
				old_str = '_old'
			elif arg.count('q') == len(arg):
				quiet_level = len(arg)
			elif arg == 'noc':
				with_color = False
			elif arg.startswith('exp='):
				file_basename_exp = arg.split('=', 1)[1]
				file_basename_exp = re.sub('[^*a-zA-Z0-9_-]', '', file_basename_exp)
			elif arg.isdigit():
				if arg[0] not in versions or arg[1:] not in versions[arg[0]]:
					print_error_and_exit('[-] Version not supported')
				else:
					versions = {arg[0]: [arg[1:]]}
	
	if with_color:
		global colored
		from colorama import just_fix_windows_console
		from termcolor import colored
		just_fix_windows_console()
	
	pycdc_path = run_cmd('where pycdc%s' % old_str, True).split('\n')[0]
	return file_basename_exp, pycdc_path, versions

def main():
	global max_align_need
	global source_files_contents
	
	file_basename_exp, pycdc_path, versions = init_and_get_args()
	
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
	compile(input_dir, compiled_dir, versions, file_basename_exp)
	decompiled_dir = Path('./decompiled/')
	decompile(compiled_dir, decompiled_dir, versions, file_basename_exp, pycdc_path)
	
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
					print_info('+', 'Succeeded', source_file.name, max_align_need, GREEN_COLOR)
				version_to_decompiled_count[(py_major_ver, py_minor_ver)] += 1
			if quiet_level < 2:
				print()
	if quiet_level < 2:
		print()
	print('Finished in %s seconds.' % my_colored('%.2f' % (time.time() - tests_starting_time), LCYAN_COLOR))
	print_summary(version_to_decompiled_count, len(input_files))


if __name__ == '__main__':
	main()
		

