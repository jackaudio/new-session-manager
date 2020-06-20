##############################################################################
# Copyright (C) 2020- Nils Hilbricht
#
# This file is part of New-Session-Manager
#
# New-Session-Manager is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# New-Session-Manager is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.
##############################################################################

project('new-session-manager', 'c', 'cpp', version : '1.3.1', license : 'GPLv3')

##############
#Dependencies
##############

liblodep = dependency('liblo') #and not 'lo'
threaddep = dependency('threads')
jackdep = dependency('jack', required: get_option('jackpatch')) #and not 'libjack'

cc = meson.get_compiler('c')
fltkdep = cc.find_library('fltk', required: get_option('nsm-legacy-gui') or get_option('nsm-proxy'))
fluid = find_program('fluid', required: get_option('nsm-proxy'))


##############
#Build Targets
##############

executable('nsmd',
    sources: ['src/nsmd.cpp', 'src/debug.cpp', 'src/Endpoint.cpp', 'src/file.cpp', 'src/Thread.cpp'],
    dependencies: [liblodep, threaddep],
    install: true,
    )


#For options see meson_options.txt
#All get_options are default=true

if get_option('jackpatch')

    executable('jackpatch',
        'src/jackpatch.c',
        dependencies: [liblodep, jackdep],
        install: true,
        )
endif


if get_option('nsm-proxy')

    NSM_Proxy_UI_cpp = custom_target(
        'NSM_Proxy_UI.cpp',
        output : 'NSM_Proxy_UI.C',
        input : 'src/NSM_Proxy_UI.fl',
        command : [fluid, '-c', '-o', '@OUTPUT@', '@INPUT@'],
    )

    NSM_Proxy_UI_h = custom_target(
        'NSM_Proxy_UI.h',
        output : 'NSM_Proxy_UI.H',
        input : 'src/NSM_Proxy_UI.fl',
        command : [fluid, '-c', '-h', '@OUTPUT@', '@INPUT@'],
    )


    executable('nsm-proxy',
        sources: ['src/nsm-proxy.cpp', 'src/debug.cpp'],
        dependencies: [liblodep, threaddep],
        install: true,
    )

    executable('nsm-proxy-gui',
        sources: ['src/nsm-proxy-gui.cpp', [NSM_Proxy_UI_cpp, NSM_Proxy_UI_h]],
        dependencies: [fltkdep, liblodep, threaddep],
        install: true,
    )
endif

if get_option('nsm-legacy-gui')

    executable('nsm-legacy-gui',
        sources: ['src/legacy-gui.cpp', 'src/debug.cpp', 'src/Endpoint.cpp', 'src/Thread.cpp', 'src/FL/Fl_Scalepack.C'],
        dependencies: [fltkdep, liblodep, threaddep],
        install: true,
        )

    install_data('src/org.linuxaudio.nsm-legacy-gui.desktop', install_dir : get_option('datadir') / 'applications')

    meson.add_install_script('sh', '-c',
        'ln -sf nsm-legacy-gui ${DESTDIR}@0@/@1@/non-session-manager'.format(
            get_option('prefix'), get_option('bindir')))

endif
