import unittest
import os
import glob
import sys

def get_all_test_modules():
	test_files = [os.path.basename(tf) for tf in glob.glob("test/*test*.py")]
	test_modules = [__import__(os.path.splitext(tf)[0]) for tf in test_files 
			if tf != 'run_tests.py']
	return test_modules

# Get test modules from the command line
test_modules = [__import__(m + '_tests') for m in sys.argv[1:]]

# if no test modules are specified, run all tests
if len(test_modules) == 0:
	test_modules = get_all_test_modules()

tests = [unittest.defaultTestLoader.loadTestsFromModule(tm) for tm in test_modules]

test_suite = unittest.TestSuite()
test_suite.addTests(tests)

unittest.TextTestRunner().run(test_suite)
