#! /usr/bin/env python
# encoding: utf-8
from waflib.Tools.compiler_cxx import cxx_compiler
from waflib.Tools.compiler_c import c_compiler

# from scripts.waf import utils

# import sys

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
    cnf.env.append_value('CXXFLAGS', ['-msse3', '-std=c++17', '-Wall', '-Werror', '-Wextra', '-O3'])
    cnf.env.append_value('CFLAGS',
                         ['-msse3', '-Wall', '-Werror', '-Wextra', '-O3',
                          '-DNDEBUG', '-fPIC'])
    

def build(bld):

    bld(name = 'libcorrect_includes',
        includes='./include',
        export_includes='./include')

    bld.stlib(name = 'libcorrect',
        features = 'c cstlib',
        target='staticcorrect',
        includes='../include',
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
 
   
    # Build Benchmark

