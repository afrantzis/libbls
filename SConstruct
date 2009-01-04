import os
import commands

env = Environment(ENV = os.environ) 
Export('env')

version = {'major':'0.1', 'minor':'1', 'patch':'0'}

libbless_name_no_lib = 'bless-%s' % version['major'] 
libbless_name = 'lib%s' % libbless_name_no_lib
libbless_soname = '%s.so.0' % libbless_name
if env['SHLIBSUFFIX'] == '.so':
	libbless_filename = libbless_soname

Export('libbless_name_no_lib')
Export('libbless_soname')
Export('libbless_filename')

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
	
# TODO: Find better (automatic) way to get this path
PYTHON_INCLUDE_PATH = '/usr/include/python2.5'

Export('PYTHON_INCLUDE_PATH')

####################################
# Autotool like installation paths #
####################################

destdir = ARGUMENTS.get('destdir', '')
prefix = ARGUMENTS.get('prefix', '/usr/local')
exec_prefix = ARGUMENTS.get('exec_prefix', prefix)

libdir = ARGUMENTS.get('libdir', os.path.join(exec_prefix, 'lib'))
includedir = ARGUMENTS.get('includedir', os.path.join(prefix, 'include', libbless_name, 'libbless'))

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

# Whether to install symbolic links for the library
install_links = ARGUMENTS.get('install-links', 'no')

###########
# Targets #
###########

# The libbless target includes the libless library and symbolic links to it.
# libbless[0] is the library, libbless[1] is the soname link, libbless[2] is
# the plain '.so' link used for development
libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0)

env.Default(libbless)
env.Alias('libbless', libbless)

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings', duplicate=0)

tests = env.SConscript('test/SConscript', build_dir='build/tests', duplicate=0)

# Install targets
install_lib = env.Install(destdir + libdir, libbless[0])

install_run_link = env.Install(destdir + libdir, libbless[1])
install_dev_link = env.Install(destdir + libdir, libbless[2])

install_headers = env.Install(destdir + includedir, ['src/buffer.h','src/buffer_source.h'])

install_targets = [install_lib, install_headers]

if install_links != 'no':
	install_targets += install_run_link
if install_links == 'yes':
	install_targets += install_dev_link

env.Alias('install',install_targets)

# Test target
AlwaysBuild(tests)
env.Alias('test', tests)

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
includedir  $$prefix/include/%(libbless_name)s/libbless
datarootdir $$prefix/share
datadir     $$datarootdir
docdir      $$datarootdir/doc/%(libbless_name)s

=== Installation options ===

install-links = yes|no-dev|no [default = no]
    Install ldconfig-like symbolic links for the library.
    yes: install all links
    no-dev: install only links needed for running linked programs
            not for compiling new applications.
    no: do not install links.
    eg scons install install-links=no-dev

=== Build options ==

debug = 0|1 [default = 0]
    Build with debug information.
    eg scons debug=1 

lfs = 0|1 [default = 1]
    Build with LFS (large file) support.
	eg scons lfs=0

=== Testing options ===

tests = LIST
   Run tests for only the selected modules.for (by default all 
   tests are run).
       eg scons debug=1 test tests=segment,buffer
""" % locals())
