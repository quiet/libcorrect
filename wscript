#! /usr/bin/env python
# encoding: utf-8
from waflib.Tools.compiler_cxx import cxx_compiler
# from scripts.waf import utils

# import sys

APPNAME = 'libcodes'
VERSION = '1.0.0'

cxx_compiler['linux'] = ['clang++']

def options(opt):
    opt.load('compiler_cxx')

def configure(cnf) :
    # TODO FIGURE OUT HOW THIS WORKS, cause it is bloody awesome
    #cnf.check(lib=['cryptopp', 'pqxx', 'pq'])
    cnf.check

    cnf.load('compiler_cxx')

    link_flags = ['-pthread']
    cxx_flags = ['-std=c++17', '-Wall', '-Werror', '-Wextra', '-O3']
    

def build(bld):

    bld(name = 'libcorrect_includes',
        includes='./include',
        export_includes='./include')

    bld.stlib(name = 'libcorrect',
        features = 'cxx cxxstlib',
        target='libcorrect',
        includes='../include',
        source=bld.path.ant_glob('src/**/*.c'),
        use=['libcorrect_includes']
    )    

    bld.shlib(name = 'libcorrect_shared',
        features = 'cxx',
        target='libcorrect_shared',
        includes='../include',
        source=bld.path.ant_glob('src/**/*.c'),
        use=['libcorrect_includes']
    )    
    
    # Build Test

    # Build Examples
 
   
    # Build Benchmark

