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

project('new-session-manager', 'c', 'cpp', version : '1.3.0', license : 'GPLv3')

liblodep = dependency('liblo') #and not 'lo'
threaddep = dependency('threads')
jackdep = dependency('jack') #and not 'libjack'

executable('nsmd',
    sources: ['src/nsmd.cpp', 'src/debug.cpp', 'src/Endpoint.cpp', 'src/file.cpp', 'src/Thread.cpp'],
    dependencies: [liblodep, threaddep],
    install: true,
    )

executable('jackpatch',
    'src/jackpatch.c',
    dependencies: [liblodep, jackdep],
    install: true,
    )



