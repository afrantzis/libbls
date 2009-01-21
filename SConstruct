import SCons
import os
import commands
import scons_helpers

##################################
# Construction environment setup #
##################################

env = Environment(ENV = os.environ) 

scons_helpers.register_builders(env)

env['libbless_major'] = '0.1'
env['libbless_minor'] = '0'
env['libbless_patch'] = '0'
env['libbless_version'] = '${libbless_major}.${libbless_minor}'

env['libbless_name_no_lib'] = 'bless-%s' % env['libbless_major']
env['libbless_name'] = 'lib%s' % env['libbless_name_no_lib']
env['libbless_soname'] = '%s.so.0' % env['libbless_name']
if env['SHLIBSUFFIX'] == '.so':
	env['libbless_filename'] = env['libbless_soname']
else:
	env['libbless_filename'] = '${libbless_name}${SHLIBSUFFIX}'

# TODO: Find better (automatic) way to get this path
env['PYTHON_INCLUDE_PATH'] = '/usr/include/python2.5'

################
# System check #
################

run_conf = not env.GetOption('clean') and not env.GetOption('help')
run_conf = run_conf and not 'dist' in BUILD_TARGETS

if run_conf == True:
	conf = Configure(env)

	req_headers = ['stdint.h', 'stdlib.h', 'string.h', 'unistd.h',
		'sys/types.h', 'fcntl.h']
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

	opt_funcs = ['posix_fallocate']
	for func in opt_funcs:
		if conf.CheckFunc(func):
			env.Append(CCFLAGS='-DHAVE_%s' % func.upper())
	
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
	# Save flags in their own construction environment variables so they can be
	# use in creating the pkg-config file
	env['lfs_cflags'] = commands.getoutput("getconf LFS_CFLAGS")
	env['lfs_ldflags'] = commands.getoutput("getconf LFS_LDFLAGS")
	env.Append(CCFLAGS = '$lfs_cflags') 
	env.Append(LINKFLAGS = '$lfs_ldflags') 
else:
	env['lfs_cflags'] = ''
	env['lfs_ldflags'] = ''

# Whether to install symbolic links for the library
install_links = ARGUMENTS.get('install-links', 'no')

#################
# Build Targets #
#################

# The libbless targetr include the libless library and symbolic links to it.
# libbless[0] is the library, libbless[1] is the soname link, libbless[2] is
# the plain '.so' link used for development

libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0, exports=['env'])

# This environment produces a library suitable for release
env_release = env.Clone()
env_release.Append(CCFLAGS='-fvisibility=hidden')
libbless_release = env_release.SConscript('src/SConscript', build_dir='build/src-release/',
	duplicate=0, exports={'env':env_release})

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings',
		duplicate=0, exports=['env'])

pkgconf = env.Template('${libbless_name_no_lib}.pc', 'bless.pc.in') 

env.Alias('libbless', libbless_release)
Depends(bindings, libbless)

env.Default([libbless_release, pkgconf])

########################
# Installation Targets #
########################

lib_targets = scons_helpers.install_versioned_library('${destdir}${libdir}', 
		libbless_release[0], env.subst('$libbless_soname'), env)

install_lib = lib_targets[0]
install_run_link = lib_targets[1]
install_dev_link = lib_targets[2]

install_headers = env.Install('${destdir}${includedir}/libbless',
		['src/buffer.h','src/buffer_source.h'])

install_pkgconf = env.Install('${destdir}${libdir}/pkgconfig', pkgconf)

install_targets = [install_lib, install_headers, install_pkgconf]

if install_links != 'no':
	install_targets += install_run_link
if install_links == 'yes':
	install_targets += install_dev_link

env.Alias('install', install_targets)

##################
# Testing Target #
##################

tests = env.SConscript('test/SConscript', build_dir='build/tests', duplicate=1, exports=['env'])

AlwaysBuild(tests)
env.Alias('test', tests)
Depends(tests, bindings)

#########################
# Documentation targets #
#########################

user_api_doc = env.Command('doc/user/html', 'Doxyfile.user', 'doxygen $SOURCE')
env.Depends(user_api_doc, scons_helpers.get_files('src', include = ['buffer*'], exclude = ['buffer_util*', '.*', '*~']))

dev_api_doc = env.Command('doc/devel/html', 'Doxyfile.devel', 'doxygen $SOURCE')

env.Depends(dev_api_doc, scons_helpers.get_files('src', exclude = ['.*', '*~']))

user_doc = env.SConscript('doc/user/SConscript', exports=['env'])

all_doc = [user_api_doc, dev_api_doc, user_doc]
env.Alias('doc', all_doc)

###############
# Dist Target #
###############

dist_files = scons_helpers.get_files('.',
	exclude = ['build', '*.log', '.*', '*~', '*.pyc', '*.gz'])

dist_archive = env.Archive('libbless-${libbless_version}.tar.gz', dist_files)

env.Alias('dist', dist_archive)
env.Depends(dist_archive, all_doc)

########################
# Benchmarking targets #
########################

benchmarks = env.SConscript('benchmarks/SConscript', build_dir='build/benchmarks/', duplicate=0, exports=['env'])
env.Depends(benchmarks, libbless_release)

run_benchmarks = env.Alias('benchmark', benchmarks, benchmarks)
env.AlwaysBuild(run_benchmarks)

########
# Help #
########

env.Help("""
Usage: scons [target] [options].

=== Targets ===

libbless: Builds libbless (default).
install: Installs libbless.
test: Runs libbless tests.
doc: Creates libbless documentation.
benchmark: Run some benchmarks.
dist: Creates a source distribution archive.

=== Installation path configuration options ===

Option      Default Value
------      -------------
destdir     
prefix      /usr/local
exec_prefix $$prefix
libdir      $$exec_prefix/lib
includedir  $$prefix/include/%(libbless_name)s/
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

=== Benchmark options ===

valgrind = 0|1 [default = 0]
    Run benchmarks under valgrind.
""" % env)

