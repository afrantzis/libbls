from string import Template

class AtTemplate(Template):
	delimiter = '@';

# A builder that replaces variables in a file
# Adapted from http://www.scons.org/wiki/ReplacementBuilder
def register_template_builder(env):
	action = env.Action(replace_action, replace_string)
	env['BUILDERS']['Template'] = env.Builder(action=action, src_suffix='.in',
			single_source=True)

def replace_action(target, source, env):
	fin = open(str(source[0]), 'r')
	fout = open(str(target[0]), 'w')
	fout.write(AtTemplate(fin.read()).safe_substitute(env))
	return 0
 
def replace_string(target, source, env):
	return "Creating '%s' from '%s'" % (str(target[0]), str(source[0]))
 

