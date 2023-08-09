#!/bin/python3

from MyTests.MyUtil import run_cmd
import glob
from pathlib import Path
import os

if not os.path.isdir('./tokenized/'):
	os.makedirs('./tokenized/')

print('starting...')
for py_input in glob.glob('./input/*.py'):
	tokenized = run_cmd('../scripts/token_dump %s' % py_input, True)
	tokenized_path = Path('./tokenized').joinpath(Path(py_input).stem + '.txt')
	with open(tokenized_path, 'w') as f:
		f.write(tokenized)
print('done')