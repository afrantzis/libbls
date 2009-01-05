from string import Template
import os
import fnmatch
import tarfile, zipfile

def register_builders(env):
	register_template_builder(env)
	register_archive_builder(env)

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

