#!/bin/python3

from subprocess import Popen, PIPE
import glob
from pathlib import Path
import os

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

if not os.path.isdir('./tokenized/'):
	os.makedirs('./tokenized/')

print('starting...')
for py_input in glob.glob('./input/*.py'):
	tokenized = run_cmd('../scripts/token_dump %s' % py_input, True)
	tokenized_path = Path('./tokenized').joinpath(Path(py_input).stem + '.txt')
	with open(tokenized_path, 'w') as f:
		f.write(tokenized)
print('done')