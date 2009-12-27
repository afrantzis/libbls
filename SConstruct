import SCons
import os
import commands
import scons_helpers

##################################
# Construction environment setup #
##################################

env = Environment(ENV = os.environ) 

scons_helpers.register_builders(env)

env['lib_major'] = '0'
env['lib_minor'] = '3'
env['lib_patch'] = '0'
env['lib_version'] = '${lib_major}.${lib_minor}.${lib_patch}'

env['lib_name_no_lib'] = 'bls-%s' % env.subst('${lib_major}.${lib_minor}')
env['lib_name'] = 'lib%s' % env['lib_name_no_lib']
env['lib_soname'] = '%s.so.0' % env['lib_name']
if env['SHLIBSUFFIX'] == '.so':
	env['lib_filename'] = env['lib_soname']
else:
	env['lib_filename'] = '${lib_name}${SHLIBSUFFIX}'

# TODO: Find better (automatic) way to get this path
env['PYTHON_INCLUDE_PATH'] = '/usr/include/python2.5'

################
# System check #
################

run_conf = not env.GetOption('clean') and not env.GetOption('help')
run_conf = run_conf and not 'dist' in BUILD_TARGETS

if run_conf == True:
	conf = Configure(env, custom_tests = scons_helpers.get_configure_tests())

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
	
	pkgs = ['lua5.1']
	for pkg in pkgs:
		if conf.CheckPKG(pkg):
			tmp_env = Environment()
			tmp_env.ParseConfig('pkg-config --cflags --libs %s' % pkg)
			env['lua_CPPPATH'] = tmp_env['CPPPATH']
			env['lua_LIBS'] = tmp_env['LIBS']
			
	env = conf.Finish()

####################################
# Autotool like installation paths #
####################################

env['destdir'] = ARGUMENTS.get('destdir', '')
env['prefix'] = ARGUMENTS.get('prefix', '/usr/local')
env['exec_prefix'] = ARGUMENTS.get('exec_prefix', '${prefix}')

env['libdir'] = ARGUMENTS.get('libdir', '${exec_prefix}/lib')
env['includedir'] = ARGUMENTS.get('includedir', 
		'${prefix}/include/' + env['lib_name_no_lib'])

env['datarootdir'] = ARGUMENTS.get('datarootdir', '${prefix}/share')
env['datadir'] = ARGUMENTS.get('datadir', '${datarootdir}')
env['docdir'] = ARGUMENTS.get('docdir',
		'%{datarootdir}/doc/' + env['lib_name'])

################################
# Various Command line options #
################################

# Add default CCFLAGS
env.Append(CCFLAGS = '-std=c99 -D_XOPEN_SOURCE=600 -Wall -Wextra -pedantic -O2')

debug = ARGUMENTS.get('debug', 0)
if int(debug):
	env.Append(CCFLAGS = '-DENABLE_DEBUG=1 -g -O0')

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
install_links = ARGUMENTS.get('install-links', 'yes')

#################
# Build Targets #
#################

# The lib target include the library and symbolic links to it.
# lib[0] is the library, lib[1] is the soname link, lib[2] is
# the plain '.so' link used for development

lib = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0, exports=['env'])

# This environment produces a library suitable for release
env_release = env.Clone()
env_release.Append(CCFLAGS='-fvisibility=hidden')
lib_release = env_release.SConscript('src/SConscript', build_dir='build/src-release/',
	duplicate=0, exports={'env':env_release})
	
bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings',
		duplicate=0, exports=['env'])
		
pkgconf = env.Template('${lib_name_no_lib}.pc', 'bls.pc.in') 

env.Alias('bindings', bindings)
env.Alias('lib', lib_release)
Depends(bindings, lib)

env.Default([lib_release, pkgconf])

########################
# Installation Targets #
########################

lib_targets = scons_helpers.install_versioned_library('${destdir}${libdir}', 
		lib_release[0], env.subst('$lib_soname'), env)

install_lib = lib_targets[0]
install_run_link = lib_targets[1]
install_dev_link = lib_targets[2]

install_headers = env.Install('${destdir}${includedir}/bls',
		['src/buffer.h','src/buffer_source.h','src/buffer_options.h',
		 'src/buffer_event.h'])

install_pkgconf = env.Install('${destdir}${libdir}/pkgconfig', pkgconf)

install_targets = [install_lib, install_headers, install_pkgconf]

if install_links != 'no':
	install_targets += install_run_link
if install_links == 'yes':
	install_targets += install_dev_link

env.Alias('install', install_targets)
env.AlwaysBuild(install_targets)

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
	exclude = ['build', 'html', '*.log', '.*', '*~', '*.pyc', '*.gz'])

dist_archive = env.Archive('libbls-${lib_version}.tar.gz', dist_files)

env.Alias('dist', dist_archive)

########################
# Benchmarking targets #
########################

benchmarks = env.SConscript('benchmarks/SConscript', build_dir='build/benchmarks/', duplicate=0, exports=['env'])
env.Depends(benchmarks, lib_release)

run_benchmarks = env.Alias('benchmark', benchmarks)
env.AlwaysBuild(run_benchmarks)

########
# Help #
########

env.Help("""
Usage: scons [target] [options].

=== Targets ===

lib: Builds library (default).
install: Installs library.
bindings: Builds bindings.
test: Runs library tests.
doc: Creates library documentation.
benchmark: Run some benchmarks.
dist: Creates a source distribution archive.

=== Installation path configuration options ===

Option      Default Value
------      -------------
destdir     
prefix      /usr/local
exec_prefix $$prefix
libdir      $$exec_prefix/lib
includedir  $$prefix/include/%(lib_name)s/
datarootdir $$prefix/share
datadir     $$datarootdir
docdir      $$datarootdir/doc/%(lib_name)s

=== Installation options ===

install-links = yes|no-dev|no [default = yes]
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
	
bindings = LIST
    Build bindings for the specified languages. Available languages
    are 'lua' and 'python'.
    eg scons bindings bindings=lua

=== Testing options ===

tests = LIST
   Run tests for only the selected modules (by default all 
   tests are run).
       eg scons debug=1 test tests=segment,buffer

=== Benchmark options ===

valgrind = 0|1 [default = 0]
    Run benchmarks under valgrind.
""" % env)

