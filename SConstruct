env = Environment()
Export('env')

PYTHON_INCLUDE_PATH = '/usr/include/python2.5'
Export('PYTHON_INCLUDE_PATH')

libbless = env.SConscript('src/SConscript', build_dir='build/src/', duplicate=0)

bindings = env.SConscript('bindings/SConscript', build_dir='build/bindings', duplicate=0)

Depends(bindings, libbless)
