
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

#include <vector>
#include <list>

#include <sigc++/sigc++.h>

#include "common.h"

using std::vector;
using std::list;
using namespace sigc;

class sequence : public trackable {

    struct data {
        vector <int> phrases;
        int num;

        data() { num = 0; }
    };

    list <data *> _history;

    data *_rd;
    data *_rw;

    char * _name;
    char * _notes;

    mutable volatile int _playing;
    mutable volatile tick_t _index;

    void lock ( void );
    void unlock ( void );

    vector <int>::iterator _find ( int n );

    void _swap ( int n1, int n2 );

public:

    signal <void> signal_new_song;

    sequence( void );

    void reset ( void );

    void insert ( unsigned int n, int pn );
    void remove ( int n );
    void move ( int n, int dir );
    int phrases ( void ) const;
    char * dump ( void );
    bool load ( const char *name );
    void save ( const char *name ) const;
    void play ( tick_t start, tick_t end ) const;
    int playing ( void ) const;
    tick_t index ( void ) const;
    tick_t length ( void ) const;

    char * name ( void ) const;
    void name ( const char *s );
    char * notes ( void ) const;
    void notes ( const char *s );

};
