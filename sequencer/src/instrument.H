
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

#include <list>
using std::list;

#include <MIDI/midievent.H>

struct i_map {
  char *name;
  unsigned char velocity;
};

class Instrument
{

    static list <Instrument *> instruments;

    char *_name;
    int _height;
    mutable bool _dirty;

    struct i_map _map[128];                                     /* note / velocity mappings */

    Instrument ( const char *name );
    bool read ( const char *s );
    bool write ( const char *s ) const;

public:

    static Instrument * open ( const char *name );
    static char ** listing ( void );

    /* mutation */
    void note ( int from, int to );
    void velocity ( int n, int v );
    void note_name ( int n, char *s );

    /* inspection */
    bool translate ( MIDI::midievent *e ) const;
    const char * note_name ( int n ) const;
    int height ( void ) const;
    const char * name ( void ) const;
    int velocity ( int n ) const;

    void save ( void ) const;

};
