import fnmatch
import os
import subprocess
import sys

class termcolors:
	OKBLUE = '\033[94m'
	OKGREEN = '\033[92m'
	WARNING = '\033[93m'
	FAIL = '\033[91m'
	ENDC = '\033[0m'
	
def ok(message):
	return termcolors.OKGREEN + message + termcolors.ENDC

def error(message):
	return termcolors.FAIL + message + termcolors.ENDC

def file_to_string(path):
	with open(path, 'r') as file:
		return file.read()
		
def run_test(test):
	cmd = ['./norn', test]
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	return p.communicate()

tests = []
for root, dirnames, filenames in os.walk('test'):
	for filename in fnmatch.filter(filenames, '*.norn'):
		tests.append(os.path.join(root, filename))
		
outs = [file.replace('.norn', '.out') for file in tests]
errs = [file.replace('.norn', '.err') for file in tests]

for test, out, err in zip(tests, outs, errs):
	if not os.path.exists(out):
		print('warning, missing out file %s' % (out))
	else:
		sys.stdout.write('running %s...' % (test))
		
		test_out, test_err = run_test(test)
		out_out = file_to_string(out)
		
		if test_out == out_out:
			print(ok('passed'))
		else:
			print(error('failed'))