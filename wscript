#! /usr/bin/env python
# encoding: utf-8
from waflib.Tools.compiler_cxx import cxx_compiler
from waflib.Tools.compiler_c import c_compiler

# from scripts.waf import utils

import sys

APPNAME = 'libcodes'
VERSION = '1.0.0'

cxx_compiler['linux'] = ['clang++']
c_compiler['linux'] = ['clang']

def options(opt):
    opt.load('compiler_cxx')
    opt.load('compiler_c')    

def configure(cnf) :
    # TODO FIGURE OUT HOW THIS WORKS, cause it is bloody awesome
    #cnf.check(lib=['cryptopp', 'pqxx', 'pq'])


    cnf.load('compiler_cxx')
    cnf.load('compiler_c')
    
    cnf.env.append_value('LINKFLAGS', ['-pthread'])
    cnf.env.append_value('CXXFLAGS', ['-std=c++17', '-Wall', '-Werror', '-Wextra', '-O3'])
    cnf.env.append_value('CFLAGS',
                         ['-Wall', '-Werror', '-Wextra', '-O3',
                          '-DNDEBUG', '-fPIC'])

    if sys.platform == 'darwin':
        
        cnf.env.append_value('CXXFLAGS', ['-stdlib=libc++'])

        import platform
        macos_ver = platform.mac_ver()[0]
        if not macos_ver.startswith('10.15'):
            cnf.env.append_value('LINKFLAGS', ['-lc++fs', '-L/usr/local/opt/llvm/lib'])
        else:
            cnf.env.append_value('CXXFLAGS', ['-isysroot/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk', '-I/usr/local/include'])
            cnf.env.append_value('CFLAGS', ['-isysroot/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk', '-I/usr/local/include'])
            
    

def build(bld):

    bld(name = 'libcorrect_includes',
        includes='./include',
        export_includes='./include')

    bld.stlib(name = 'libcorrect_static',
        features = 'c cstlib',
        target='libcorrect_static',
        includes='include/libcorrect',
        source=bld.path.ant_glob('src/**/*.c'),
        use=['libcorrect_includes']
    )    

    # bld.shlib(name = 'libcorrect_shared',
    #     features = 'c',
    #     target='libcorrect_shared',
    #     includes='../include',
    #     source=bld.path.ant_glob('src/**/*.c'),
    #     use=['libcorrect_includes']
    # )    
    
    # Build Test

    # Build Examples
    bld.recurse('examples/basic_example')
   
    # Build Benchmark

