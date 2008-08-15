import os

env = Environment(ENV = os.environ)
Export('env')

PYTHON_INCLUDE_PATH = '/usr/include/python2.5'
Export('PYTHON_INCLUDE_PATH')

libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0)

env.Default(libbless)

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings', duplicate=0)

tests = env.SConscript('test/SConscript', build_dir='build/tests', duplicate=0)

AlwaysBuild(tests)
env.Alias('test', tests)

Depends(tests, bindings)
Depends(bindings, libbless)
