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
    opt.add_option('--disable-sse', action='store_false', default=True, dest='sse',
                    help='Disable SSE optimization')
    opt.add_option('--project', action='store', default=False, dest='project',
                    help='Limit build to a single project (' + ', '.join( projects ) + ')')

    for i in projects:
        opt.recurse(i)
    
def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('gnu_dirs')
    conf.load('ntk_fluid')
    conf.load('gccdeps')
    conf.line_just = 52

    conf.env['LIB_PTHREAD'] = ['pthread']
    conf.env['LIB_DL'] = ['dl']
    conf.env['LIB_M'] = ['m']
  
    if Options.options.sse:
	if os.system("grep -q '^flags.*sse' /proc/cpuinfo"):
	   Options.options.sse = 0
	   print "Processor lacks sse, disabling..."

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

    conf.check_cc(msg='Checking for jack_port_get_latency_range()',
                  define_name='HAVE_JACK_PORT_GET_LATENCY_RANGE',
                  fragment='#include <jack/jack.h>\nint main (int argc, char**argv) { jack_port_get_latency_range( (jack_port_t*)0, JackCaptureLatency, (jack_latency_range_t *)0 ); }',
                  mandatory=False);
		
    conf.check(function_name='jack_get_property',
               header_name='jack/metadata.h',
               define_name='HAVE_JACK_METADATA',
               uselib='JACK',
               mandatory=False)

    conf.check_cfg(package='x11', uselib_store='XLIB',args="--cflags --libs",
                      mandatory=True)

    conf.check_cfg(package='liblo', uselib_store='LIBLO',args="--cflags --libs",
                      atleast_version='0.26', mandatory=True)

    conf.check_cc(msg='Checking for compiler pointer alignment hints',
                  uselib_store='HAS_BUILTIN_ASSUME_ALIGNED',
                  fragment='int main ( char**argv, int argc ) { const char *s = (const char*)__builtin_assume_aligned( 0, 16 ); return 0; }',
                  execute=False, mandatory=False)
###

    for i in common:
        conf.recurse(i)

    optimization_flags = [
            "-O3",
            "-fomit-frame-pointer",
            "-ffast-math",
            "-pipe"
            ]
    
    if Options.options.sse:
        print('Using SSE optimization')
        optimization_flags.extend( [
            "-msse2",
            "-mfpmath=sse" ] );

        conf.define( 'USE_SSE', 1 )

    debug_flags = [ '-O0', '-g3' ]

    if Options.options.debug:
        print('Building for debugging')
        conf.env.append_value('CFLAGS', debug_flags )
        conf.env.append_value('CXXFLAGS', debug_flags )
    else:
        print('Building for performance')
        conf.env.append_value('CFLAGS', optimization_flags )
        conf.env.append_value('CXXFLAGS', optimization_flags )
        conf.define( 'NDEBUG', 1 )

    conf.define( "_GNU_SOURCE", 1)

    conf.env.append_value('CFLAGS',['-Wall'])
#    conf.env.append_value('CXXFLAGS',['-Wall','-fno-exceptions', '-fno-rtti'])
    conf.env.append_value('CXXFLAGS',['-Wall','-fno-rtti'])

    global_flags = [ '-pthread',
                     '-D_LARGEFILE64_SOURCE',
                     '-D_FILE_OFFSET_BITS=64',
                     '-D_GNU_SOURCE' ]

    
    conf.env.append_value('CFLAGS', global_flags )
    conf.env.append_value('CXXFLAGS', global_flags )


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
    
