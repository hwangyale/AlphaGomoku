from distutils.core import setup, Extension

sources = ['board_wrap.cxx', 'board.cpp', 'gomoku_table.cpp',
           'bit_board.cpp', 'vct.cpp',
           'evaluate.cpp', 'bit_board_type.cpp', 'table.cpp']

module = Extension('_board', sources=sources)

setup(name='board',
      ext_modules=[module],
      py_modules=['board'])
