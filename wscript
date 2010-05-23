import commands
import subprocess
import os
import Options
import Scripting

blddir = 'build'

top = '.'
VERSION = '0.3.0'
APPNAME = 'libbls'

def option_list_cb(option, opt, value, parser):
	value = value.split(',')
	setattr(parser.values, option.dest, value)
	
def set_options(opt):
	opt.tool_options('compiler_cc')

	opt.add_option('--debug', action='store_true', help='build with debug information')
	opt.add_option('--no-lfs', action='store_false', dest = 'lfs', default = True,
			help='disable Large File Support')
	opt.add_option('--bindings', type = 'string', action = 'callback', callback = option_list_cb, help='the bindings to build')
	opt.add_option('--tests', type = 'string', action = 'callback', callback = option_list_cb, help='the tests to run')
	opt.add_option('--benchmarks', type = 'string', action = 'callback', callback = option_list_cb, help='the benchmarks to run')
	opt.parser.set_default('bindings', [])
	opt.parser.set_default('tests', [])
	
def configure(conf):
	conf.check_tool('compiler_cc')
	conf.check_tool('misc')
	
	# Check required headers
	req_headers = ['stdint.h', 'stdlib.h', 'string.h', 'unistd.h',
		'sys/types.h', 'fcntl.h']
	for header in req_headers:
		conf.check_cc(header_name = header, mandatory = True)
		
	# Check required types
	req_types = [('off_t', 'sys/types.h'), ('size_t', 'sys/types.h')]
	for type, header in req_types:
		conf.check_cc(type_name = type, header_name = header, mandatory = True)
		
	# Check required functions
	req_funcs = [('malloc', 'stdlib.h') ,('mmap', 'sys/mman.h'), ('munmap', 'sys/mman.h'),
			('realloc', 'stdlib.h'), ('ftruncate', 'unistd.h')]
	for func, header in req_funcs:
		conf.check_cc(function_name = func, header_name = header, mandatory = True)
		
	# Check optional functions
	opt_funcs = [('posix_fallocate', 'fcntl.h')]
	for func, header in opt_funcs:
		if conf.check_cc(function_name = func, header_name = header, mandatory = False):
			conf.env.append_unique('CCDEFINES', ('HAVE_%s' % func).upper())
			
	# Check optional packages
	opt_pkgs = ['lua5.1']
	for pkg in opt_pkgs:
		conf.check_cfg(package = pkg, uselib_store = 'lua', args = '--cflags --libs',
				mandatory = 'lua' in Options.options.bindings)
	
	conf.env.append_unique('CCFLAGS', '-std=c99 -D_XOPEN_SOURCE=600 -Wall -Wextra -pedantic'.split(' '))
	if Options.options.debug:
		conf.env.append_unique('CCFLAGS', ['-g', '-O0'])
	else:
		conf.env.append_unique('CCFLAGS', '-O2')
		
	# Add LFS flags
	if Options.options.lfs:
		lfs_cflags = commands.getoutput("getconf LFS_CFLAGS").split(' ');
		lfs_ldflags = commands.getoutput("getconf LFS_LDFLAGS").split(' ');
		lfs_libs = commands.getoutput("getconf LFS_LIBS").split(' ');
		conf.env.append_unique('CCFLAGS', lfs_cflags)
		conf.env.LFS_CFLAGS = lfs_cflags
		conf.env.append_unique('LINKFLAGS', lfs_ldflags)
		conf.env.append_unique('LINKFLAGS', lfs_libs)
		conf.env.LFS_LINKFLAGS = lfs_ldflags
		conf.env.append_unique('LFS_LINKFLAGS', lfs_libs)
	
	# Check for swig
	conf.find_program('swig', var = 'SWIG', mandatory = 'python' in Options.options.bindings)
	conf.env.bindings = Options.options.bindings
	conf.env.LIBBLS_VERSION = VERSION
	(conf.env.LIBBLS_VERSION_MAJOR, conf.env.LIBBLS_VERSION_MINOR, conf.env.LIBBLS_VERSION_PATCH) = VERSION.split('.')
	conf.env.LIBBLS_VERSION_NO_PATCH = '%s.%s' % (conf.env.LIBBLS_VERSION_MAJOR, conf.env.LIBBLS_VERSION_MINOR)
	
def build(bld):
	bld.recurse('src')
	bld.recurse('doc/user')
	
	if Options.options.benchmarks:
		bld.recurse('benchmarks')
		
	if Options.options.tests:
		if 'python' not in bld.env.bindings:
			bld.env.bindings.append('python')
		bld.recurse('test')
		
	if bld.env.bindings:
		bld.recurse('bindings')
		
	flat_env = {}
	for (k, v) in bld.env.get_merged_dict().items():
		if type(v).__name__ == 'list':
			flat_env[k] = " ".join(v)
		else:
			flat_env[k] = v
		
	bld(
		features = 'subst',
		source = 'bls.pc.in',
		target = 'bls-%s.pc' % bld.env.LIBBLS_VERSION_NO_PATCH,
		dict = flat_env,
		install_path = '${PREFIX}/lib/pkgconfig'
	)
										
def test(ctx):
	if not Options.options.tests:
		Options.options.tests = ['all']
	Scripting.commands += ['build']
	
def benchmark(ctx):
	if not Options.options.benchmarks:
		Options.options.benchmarks = ['all']
	Scripting.commands += ['build']

def doxygen(ctx):
	doc_path = os.path.join(blddir, 'default', 'doc')
	if not os.path.isdir(doc_path):
		os.makedirs(doc_path)

	subprocess.call(['doxygen', 'Doxyfile.user'])
	subprocess.call(['doxygen', 'Doxyfile.devel'])