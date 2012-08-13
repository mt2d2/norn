import fnmatch
import os
import subprocess
import sys
import time

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

def fast(message):
	return termcolors.OKBLUE + message + termcolors.ENDC

def slow(message):
	return termcolors.WARNING + message + termcolors.ENDC

def file_to_string(path):
	with open(path, 'r') as file:
		return file.read()
		
def run_test(test, nojit=False):
	if nojit:
		cmd = ['./norn', '-nojit', test]
	else:
		cmd = ['./norn', test]

	start = time.time()
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	stdout, stderr = p.communicate()
	end = time.time()

	return (stdout, stderr, end-start)

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
		
		test_jit_out, test_jit_err, jit_time = run_test(test)
		test_vm_out, test_vm_err, vm_time = run_test(test, nojit=True)
		out_out = file_to_string(out)

		jit_vm_ratio = (vm_time - jit_time) / vm_time
		if jit_vm_ratio >= 0:
			jit_vm_ratio_string = fast("%2.2f" % (jit_vm_ratio))
		else:
			jit_vm_ratio_string = slow("%2.2f" % (jit_vm_ratio))

		if test_jit_out == test_vm_out == out_out:
			print("%s %s" % (ok('passed'), jit_vm_ratio_string))
		else:
			print(error('failed'))