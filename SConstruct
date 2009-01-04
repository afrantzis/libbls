import os
import commands
import scons_helpers

##################################
# Construction environment setup #
##################################

env = Environment(ENV = os.environ) 

scons_helpers.register_template_builder(env)

env['libbless_major'] = '0.1'
env['libbless_minor'] = '0'
env['libbless_patch'] = '0'

env['libbless_name_no_lib'] = 'bless-%s' % env['libbless_major']
env['libbless_name'] = 'lib%s' % env['libbless_name_no_lib']
env['libbless_soname'] = '%s.so.0' % env['libbless_name']
if env['SHLIBSUFFIX'] == '.so':
	env['libbless_filename'] = env['libbless_soname']

# TODO: Find better (automatic) way to get this path
env['PYTHON_INCLUDE_PATH'] = '/usr/include/python2.5'

Export('env')

################
# System check #
################

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

####################################
# Autotool like installation paths #
####################################

env['destdir'] = ARGUMENTS.get('destdir', '')
env['prefix'] = ARGUMENTS.get('prefix', '/usr/local')
env['exec_prefix'] = ARGUMENTS.get('exec_prefix', '${prefix}')

env['libdir'] = ARGUMENTS.get('libdir', '${exec_prefix}/lib')
env['includedir'] = ARGUMENTS.get('includedir', 
		'${prefix}/include/' + env['libbless_name'])

env['datarootdir'] = ARGUMENTS.get('datarootdir', '${prefix}/share')
env['datadir'] = ARGUMENTS.get('datadir', '${datarootdir}')
env['docdir'] = ARGUMENTS.get('docdir',
		'%{datarootdir}/doc/' + env['libbless_name'])

################################
# Various Command line options #
################################

# Add default CCFLAGS
env.Append(CCFLAGS = '-std=c99 -D_XOPEN_SOURCE=600 -Wall -pedantic -O3')

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

#################
# Build Targets #
#################

# The libbless target includes the libless library and symbolic links to it.
# libbless[0] is the library, libbless[1] is the soname link, libbless[2] is
# the plain '.so' link used for development

libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0)

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings',
		duplicate=0)

pkgconf = env.Template('${libbless_name_no_lib}.pc', 'bless.pc.in') 

env.Alias('libbless', libbless)
Depends(bindings, libbless)

env.Default(libbless, pkgconf)

########################
# Installation Targets #
########################

install_lib = env.Install('${destdir}${libdir}', libbless[0])

install_run_link = env.Install('${destdir}${libdir}', libbless[1])
install_dev_link = env.Install('${destdir}${libdir}', libbless[2])

install_headers = env.Install('${destdir}${includedir}/libbless',
		['src/buffer.h','src/buffer_source.h'])

install_pkgconf = env.Install('${destdir}${libdir}/pkgconfig', pkgconf)

install_targets = [install_lib, install_headers, install_pkgconf]

if install_links != 'no':
	install_targets += install_run_link
if install_links == 'yes':
	install_targets += install_dev_link

env.Alias('install',install_targets)

##################
# Testing Target #
##################

tests = env.SConscript('test/SConscript', build_dir='build/tests', duplicate=0)

AlwaysBuild(tests)
env.Alias('test', tests)
Depends(tests, bindings)

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
""" % env)
