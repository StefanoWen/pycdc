from subprocess import Popen, PIPE
import sys
import glob
from pathlib import Path

python_versions = [
'30',
'31',
'32',
'33',
'34',
'35',
'36',
'37',
'38',
'39',
'310',
#'311',
]


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

def get_python_versions_dir():
	return Path(run_cmd('where python', True).split('\n')[0]).parent.parent

def print_info(indicate_char, info, head, max_align):
	print('[{}] {} {}--> {}'.format(
		indicate_char*2, 
		head, 
		'-'*(max_align - len(head)),
		info))

def print_start(is_decompile, file_exp):
	print('======================')
	print('Starting %scompile...' % ('de' if is_decompile else ''))
	if (is_decompile and file_exp != '*.pyc') or (not is_decompile and file_exp != '*.py'):
		print('Files expression: %s' % file_exp)
	print('======================')




