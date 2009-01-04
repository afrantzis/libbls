import os
import commands

env = Environment(ENV = os.environ) 
Export('env')

version = {'major':'0.1', 'minor':'1', 'patch':'0'}

libbless_name_no_lib = 'bless-%s' % version['major'] 
libbless_name = 'lib%s' % libbless_name_no_lib
libbless_soname = '%s.so.0' % libbless_name
libbless_filename = libbless_soname

Export('libbless_name_no_lib')

#################
# Configuration #
#################

if not env.GetOption('clean') and not env.GetOption('help'):
	conf = Configure(env)

	req_headers = ['stdint.h', 'stdlib.h', 'string.h', 'unistd.h', 'sys/types.h']
	for header in req_headers:
		if not conf.CheckCHeader(header):
			print "Missing C header file '%s'" % header
			Exit(1)

	req_types = [('off_t', 'sys/types.h'), ('size_t', 'sys/types.h')]
	for type, header in req_types:
		if not conf.CheckType(type, "#include <%s>\n" % header):
			print "Missing C typedef '%s'" % type
			Exit(1)

	req_funcs = ['malloc', 'mmap', 'munmap', 'realloc', 'ftruncate']
	for func in req_funcs:
		if not conf.CheckFunc(func):
			print "Missing C function '%s'" % func
			Exit(1)

	env = conf.Finish()

#################
# Build options #
#################

# Add default CCFLAGS and SHLINKFLAGS
env.Append(CCFLAGS = '-std=c99 -D_XOPEN_SOURCE=600 -Wall -pedantic -O3')
env.Append(SHLINKFLAGS = '-Wl,-soname=%s' % libbless_soname)
	
# TODO: Find better (automatic) way to get this path
PYTHON_INCLUDE_PATH = '/usr/include/python2.5'

# Make vars visible to SConscript files
Export('PYTHON_INCLUDE_PATH')

####################################
# Autotool like installation paths #
####################################

destdir = ARGUMENTS.get('destdir', '')
prefix = ARGUMENTS.get('prefix', '/usr/local')
exec_prefix = ARGUMENTS.get('exec_prefix', prefix)

libdir = ARGUMENTS.get('libdir', os.path.join(exec_prefix, 'lib'))
includedir = ARGUMENTS.get('includedir', os.path.join(prefix, 'include', libbless_name)) 

datarootdir = ARGUMENTS.get('datarootdir', os.path.join(prefix, 'share'))
datadir = ARGUMENTS.get('datadir', datarootdir)
docdir = ARGUMENTS.get('docdir', os.path.join(datarootdir, 'doc', libbless_name))

################################
# Various Command line options #
################################

debug = ARGUMENTS.get('debug', 0)
if int(debug):
	env.Append(CCFLAGS = '-g -O0')

# Large File Support
lfs = ARGUMENTS.get('lfs', 1)
if int(lfs):
	env.Append(CCFLAGS = commands.getoutput("getconf LFS_CFLAGS"))
	env.Append(LINKFLAGS = commands.getoutput("getconf LFS_LDFLAGS"))

###########
# Targets #
###########

libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0)

env.Default(libbless)
env.Alias('libbless', libbless)

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings', duplicate=0)

tests = env.SConscript('test/SConscript', build_dir='build/tests', duplicate=0)

# Install library and header files targets
libbless_install_path = os.path.join(destdir + libdir, libbless_filename)
install_lib = env.InstallAs(libbless_install_path, libbless)

install_headers = env.Install(destdir + includedir, ['src/buffer.h','src/buffer_source.h'])

AlwaysBuild(tests)
env.Alias('test', tests)
env.Alias('install',[install_lib, install_headers])

Depends(tests, bindings)
Depends(bindings, libbless)

########
# Help #
########

env.Help("""
Usage: scons [target] [options].

=== Targets ===

libbless: Builds libbless (default).
install: Installs libbless.
test: Runs libbless tests.

=== Installation path configuration options ===

Option      Default Value
------      -------------
destdir     
prefix      /usr/local
exec_prefix $$prefix
libdir      $$exec_prefix/lib
includedir  $$prefix/include/%(libbless_name)s
datarootdir $$prefix/share
datadir     $$datarootdir
docdir      $$datarootdir/doc/%(libbless_name)s

=== Testing options ===

tests: Specifies the components to run tests for (by default all tests are run).
    eg scons test tests=segment,buffer
""" % locals())
