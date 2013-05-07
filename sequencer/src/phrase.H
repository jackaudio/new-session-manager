
/*******************************************************************************/
/* Copyright (C) 2007,2008 Jonathan Moore Liles                                */
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

#include "grid.H"
#include "canvas.H"

class pattern;

#include <vector>
using std::vector;

class phrase : public Grid
{

    static vector <phrase*> _phrases;

    void _add ( void );

public:

    static signal <void> signal_create_destroy;

    phrase ( void );
    ~phrase ( void );
    phrase ( const phrase &rhs );

    static int phrases ( void );
    static phrase * phrase_by_number ( int n );
    static void reset ( void );

    virtual bool velocity_sensitive ( void ) const { return false; }

    phrase *create ( void );
    phrase * by_number ( int n ) const;

    virtual void put ( int x, int y, tick_t l, int velocity = 64 );
    phrase * clone ( void );
    const char * row_name ( int r ) const;
    void draw_row_names ( Canvas *c ) const;
    void play ( tick_t start, tick_t end );
    void trigger ( tick_t start, tick_t end );

    void load ( smf *f );
    void dump ( smf *f );
};
