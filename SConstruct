import os
import commands

env = Environment(ENV = os.environ) 
	
# TODO: Find better (automatic) way to get this path
PYTHON_INCLUDE_PATH = '/usr/include/python2.5'

# Make vars visible to SConscript files
Export('PYTHON_INCLUDE_PATH')
Export('env')

########################
# Command line options #
########################

debug = ARGUMENTS.get('debug', 0)
if int(debug):
	env.Append(CCFLAGS = '-g')

# Large File Support
lfs = ARGUMENTS.get('lfs', 1)
if int(lfs):
	env.Append(CCFLAGS = commands.getoutput("getconf LFS_CFLAGS"))
	env.Append(LDFLAGS = commands.getoutput("getconf LFS_LDFLAGS"))

###########
# Targets #
###########

libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0)

env.Default(libbless)

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings', duplicate=0)

tests = env.SConscript('test/SConscript', build_dir='build/tests', duplicate=0)

AlwaysBuild(tests)
env.Alias('test', tests)

Depends(tests, bindings)
Depends(bindings, libbless)
