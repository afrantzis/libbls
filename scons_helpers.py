from string import Template
import os
import fnmatch
import tarfile, zipfile

def register_builders(env):
	register_template_builder(env)
	register_archive_builder(env)
	register_localsymlink_builder(env)

class AtTemplate(Template):
	delimiter = '@';

# A builder that replaces variables in a file
# Adapted from http://www.scons.org/wiki/ReplacementBuilder
def register_template_builder(env):
	action = env.Action(template_action, template_string)
	env['BUILDERS']['Template'] = env.Builder(action=action, src_suffix='.in',
			single_source=True)

def template_action(target, source, env):
	fin = open(str(source[0]), 'r')
	fout = open(str(target[0]), 'w')
	fout.write(AtTemplate(fin.read()).safe_substitute(env))
	return 0

def template_string(target, source, env):
	return "Creating '%s' from '%s'" % (str(target[0]), str(source[0]))

def accept_pattern(name, include, exclude):
	included = False
	excluded = False
	for pattern in include:
		if fnmatch.fnmatch (name, pattern):
			included = True
			for pattern in exclude:
				if fnmatch.fnmatch (name, pattern):
					excluded = True
					break
			break

	return included and not excluded

def get_files(path, include = ['*'],  exclude= []):
	""" Recursively find files in path matching include patterns list
		and not matching exclude patterns
	"""

	ret_files = []
	for root, dirs, files in os.walk(path):

		for filename in files:
			if accept_pattern(filename, include, exclude):
				fullname = os.path.join (root, filename)
				ret_files.append(fullname)

		dirs[:] = [d for d in dirs if accept_pattern(d, include, exclude)]

	return ret_files


# A builder that creates a rooted archive
# Adapted from http://www.scons.org/wiki/ArchiveBuilder
def register_archive_builder(env):
	action = env.Action(archive_action, archive_string)
	env['BUILDERS']['Archive'] = env.Builder(action=action)

def archive_action (target, source, env):
	""" Make an archive from sources """

	path = str(target[0])
	base = os.path.basename(path)

	if path.endswith('.tgz'):
		archive = tarfile.open (path, 'w:gz')
		prefix = base[:-len('.tgz')]
	elif path.endswith('.tar.gz'):
		archive = tarfile.open (path, 'w:gz')
		prefix = base[:-len('.tar.gz')]
	elif path.endswith('.tar.bz2'):
		archive = tarfile.open (path, 'w:bz2')
		prefix = base[:-len('.tar.bz2')]
	elif path.endswith('.zip'):
		archive = zipfile.ZipFile (path, 'w')
		archive.add = archive.write
		prefix = base[:-len('.zip')]
	else:
		print "Unknown archive type (%s)" % type
		return

	src = [str(s) for s in source if str(s) != path]
	for s in src:
		archive.add (s, os.path.join (prefix, s))
	archive.close()

def archive_string (target, source, env):
	""" Information string for Archive """
	return 'Making archive %s' % os.path.basename (str (target[0]))

# A symbolic link builder that creates a symlink to a file in the 
# same directory as that source file
def register_localsymlink_builder(env):
	action = env.Action(localsymlink_action, localsymlink_string)
	env['BUILDERS']['LocalSymlink'] = env.Builder(action=action,
			emitter = localsymlink_emitter)

def localsymlink_emitter(target, source, env):
	t_base = os.path.basename(str(target[0]))
	s_dir = os.path.dirname(str(source[0]))

	return ([os.path.join(s_dir, t_base)], source)

def localsymlink_action (target, source, env):
	""" Make an symlink target pointing to source"""
	t = str(target[0])
	if os.path.lexists(t):
		if os.path.islink(t):
			os.unlink(t)
	
	os.symlink(os.path.basename(str(source[0])), str(target[0]))
	return 0

def localsymlink_string (target, source, env):
	""" Information string for symlink """
	return 'Creating symlink %s -> %s' % (str(target[0]), str(source[0]))

def create_library_links(lib, soname, env):
	# Dummy builders for links
	lnk1 = env.Command(None, None, "")
	lnk2 = env.Command(None, None, "")

	if env['SHLIBSUFFIX'] == '.so':
	
		lib_base = os.path.basename(lib)
		lnk1_name = soname
		lnk1_base = os.path.basename(lnk1_name)

		if lnk1_base != lib_base:
			lnk1 = env.LocalSymlink(lnk1_name, lib)
			env.Depends(lnk1, lib)
	
		so_index = soname.rfind('.so')
		lnk2_name = soname[:so_index + 3]
		lnk2_base = os.path.basename(lnk2_name)

		if lnk2_base != lib_base and lnk2_base != lnk1_base:
			lnk2 = env.LocalSymlink(lnk2_name, lib)
			env.Depends(lnk2, lib)

	return lnk2, lnk2

# Helper function to create a versioned library and related symlinks
def versioned_library(target, source, soname, env):
	"""Creates the shared library named 'target' from Nodes 'source'
	using the specified 'soname' and the scons 'env'"""

	env_lib = env.Clone()

	if env.subst('$SHLIBSUFFIX') == '.so':
		env_lib.Append(SHLINKFLAGS = '-Wl,-soname=%s' % soname)
		so_index = target.rfind('.so')
		env_lib['SHLIBSUFFIX'] = target[so_index:]
		env_lib.Append(LIBSUFFIXES=['$SHLIBSUFFIX'])
	
	suffix_index = target.rfind(env_lib.subst('$SHLIBSUFFIX'))
	prefix_length = len(env_lib.subst('$SHLIBPREFIX'))

	lib_name = target[prefix_length:suffix_index]
	
	lib = env_lib.SharedLibrary(lib_name, source)

	lnk1, lnk2 = create_library_links(str(lib[0]), soname, env)
	
	return [lib, lnk1, lnk2]

def install_versioned_library(target_dir, source, soname, env):
	
	lib = env.Install(target_dir, source)

	lnk1, lnk2 = create_library_links(str(lib[0]), soname, env)
	
	return [lib, lnk1, lnk2]


