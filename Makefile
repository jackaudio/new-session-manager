
###############################################################################
# Copyright (C) 2008 Jonathan Moore Liles				      #
# 									      #
# This program is free software; you can redistribute it and/or modify it     #
# under the terms of the GNU General Public License as published by the	      #
# Free Software Foundation; either version 2 of the License, or (at your      #
# option) any later version.						      #
# 									      #
# This program is distributed in the hope that it will be useful, but WITHOUT #
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or	      #
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   #
# more details.								      #
# 									      #
# You should have received a copy of the GNU General Public License along     #
# with This program; see the file COPYING.  If not,write to the Free Software #
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  #
###############################################################################

include scripts/colors

SUBDIRS=nonlib FL timeline mixer session-manager sequencer

.config: configure
	@ echo $(BOLD)$(YELLOW)Hey! You need to run 'configure' first.
	@ echo If that fails because of NTK stuff, then you need to run 'make ntk' first.$(SGR0)
	@ exit 1

all: .config
	@ echo '!!! If you have any trouble here try reading README.build !!!'
	@ for dir in $(SUBDIRS); do echo Building $$dir; $(MAKE) -s -C $$dir; done

ntk: lib/.built lib/ntk/configure

lib/ntk/configure:
	@ git submodule update --init

lib/.built: 
	@ make -C lib

clean:
	@ for dir in $(SUBDIRS); do $(MAKE) -s -C $$dir clean; done

install:
	@ for dir in $(SUBDIRS); do $(MAKE) -s -C $$dir install; done
