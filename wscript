#!/usr/bin/env python
import os
import subprocess
import waflib.Logs as Logs, waflib.Options as Options
import string

VERSION = 'xxx'
APPNAME = 'non-things'

top = '.'
out = 'build'

common = [ 'nonlib', 'FL' ]
projects = [ 'timeline', 'mixer', 'sequencer', 'session-manager' ]

def options(opt):
    # opt.add_option('--use-system-ntk', action='store_true', default=False,
    #                dest='use_system_ntk',
    #                help="Link to system-installed shared NTK instead of bundled version")    
    opt.add_option('--enable-debug', action='store_true', default=False, dest='debug',
                    help='Build for debugging')
    opt.add_option('--project', action='store', default=False, dest='project',
                    help='Limit build to a single project (' + ', '.join( projects ) + ')')

    for i in projects:
        opt.recurse(i)
    
def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('gnu_dirs')
    conf.load('ntk_fluid')
    conf.line_just = 52

    optimization_flags = [
            "-O3",
            "-fomit-frame-pointer",
            "-ffast-math",
#            "-fstrength-reduce",
            "-pipe"
            ]

    debug_flags = [ '-g' ]

    if Options.options.debug:
        conf.env.append_value('CFLAGS', debug_flags )
        conf.env.append_value('CXXFLAGS', debug_flags )
    else:
        conf.env.append_value('CFLAGS', optimization_flags )
        conf.env.append_value('CXXFLAGS', optimization_flags )
        conf.define( 'NDEBUG', 1 )

    conf.define( "_GNU_SOURCE", 1)

    conf.env.append_value('CFLAGS',['-Wall'])
#    conf.env.append_value('CXXFLAGS',['-Wall','-fno-exceptions', '-fno-rtti'])
    conf.env.append_value('CXXFLAGS',['-Wall','-fno-rtti'])


    # NTK_EXTRA_FLAGS=''
    # if not Options.options.use_system_ntk:
    #     print 'Using bundled NTK'
    #     os.environ['PKG_CONFIG_PATH'] = 'lib/ntk/build/:' + os.environ.get('PKG_CONFIG_PATH','')
    #    NTK_EXTRA_FLAGS='--static'
    #     PWD = os.environ.get('PWD','')
    #     os.environ['PATH'] = PWD + '/lib/ntk/build/fluid:' + os.environ.get('PATH','')

    conf.check_cfg(package='ntk', uselib_store='NTK', args='--cflags --libs',
                   atleast_version='1.3.0', mandatory=True)
    conf.check_cfg(package='ntk_images', uselib_store='NTK_IMAGES', args=' --cflags --libs',
                   atleast_version='1.3.0', mandatory=True)

    conf.find_program('ntk-fluid', var='NTK_FLUID')

    conf.check_cfg(package='jack', uselib_store='JACK', args="--cflags --libs",
                   atleast_version='0.103.0', mandatory=True)

    conf.check_cfg(package='xpm', uselib_store='XMP',args="--cflags --libs",
                      atleast_version='2.0.0', mandatory=True)

    conf.check_cfg(package='liblo', uselib_store='LIBLO',args="--cflags --libs",
                      atleast_version='0.26', mandatory=True)

###

    for i in common:
        conf.recurse(i)

    conf.env.PROJECT = conf.options.project

    if conf.env.PROJECT:
        pl = [ conf.env.PROJECT ]
    else:
        pl = projects;

    for i in pl:
        Logs.pprint('YELLOW', 'Configuring %s' % i)
        conf.recurse(i);

def run(ctx):
    if not Options.options.cmd:
        Logs.error("missing --cmd option for run command")
        return

    cmd = Options.options.cmd
    Logs.pprint('GREEN', 'Running %s' % cmd)

    subprocess.call(cmd, shell=True, env=source_tree_env())

def build(bld):
    
    for i in common:
        bld.recurse(i)

    if bld.env.PROJECT:
        pl = [ bld.env.PROJECT ]
    else:
        pl = projects;

    for i in pl:
        bld.recurse(i);
    
