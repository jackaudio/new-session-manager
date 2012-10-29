
/*******************************************************************************/
/* Copyright (C) 2008 Jonathan Moore Liles                                     */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

#pragma once
#include <MIDI/midievent.H>

class Scale
{

    static const char *chromatic_names[];
    static Scale builtin_scales[];

    int _degree ( int k, int n ) const;

public:
    const char *_name;
    int _notes;
    int _degrees[13];

public:

    static Scale * open ( const char *name );
    static char ** listing ( void );
    static const char * chromatic_name ( int n );
    static int octave ( int n );

    bool translate ( int k, MIDI::midievent *e ) const;
    int note ( int k, int n ) const;
    const char * note_name ( int k, int n ) const;
    const char * name ( void ) const;
};
