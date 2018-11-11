from distutils.core import setup, Extension


module = Extension('_board', sources=['board_wrap.cxx',
                                      'board.cpp',
                                      'board_type.cpp',
                                      'vct.cpp'])

setup(name='board',
      ext_modules=[module],
      py_modules=['board'])
