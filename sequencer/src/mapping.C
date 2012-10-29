
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

#include "mapping.H"
#include "stdlib.h"
#include "common.h"
#include <MIDI/midievent.H>

using namespace MIDI;
/* Is C++'s dispatching useless or what? */

#define IS_INSTRUMENT ( _type == INSTRUMENT )

Mapping::Mapping ( )
{
    _key = 0;
    _type = NONE;
}

bool
Mapping::open ( enum map_type type, const char *name )
{
    switch ( type )
    {
        case INSTRUMENT:
            _instrument = Instrument::open( name );
            break;
        case SCALE:
            _scale = Scale::open( name );
            break;
        default:
            ASSERTION( "invalid mapping type ");
            break;
    }

    if ( ! _scale )
        return false;

    _type = type;

    return true;
}

Mapping::Mapping ( const Mapping &rhs )
{
    _key = rhs._key;
    _type = rhs._type;
    _instrument = rhs._instrument;
}

const char *
Mapping::name ( void ) const
{
    switch ( _type )
    {
        case INSTRUMENT:
            return _instrument->name();
        case SCALE:
            return _scale->name();
    }

    return NULL;
}


int
Mapping::height ( void ) const
{
    if ( IS_INSTRUMENT )
        return _instrument->height();
    else
        return 0;
}

void
Mapping::key ( int n )
{
    if ( n > 11 || n < 0 )
        ASSERTION( "key selection out of range: %d", n );

    _key = n;
}

int
Mapping::key ( void ) const
{
    return _type == INSTRUMENT ? -1 : _key;
}

bool
Mapping::translate ( midievent *e ) const
{
    switch ( _type )
    {
        case INSTRUMENT:
            return _instrument->translate( e );
        case SCALE:
            return _scale->translate( _key, e );
    }

    return false;
}

int
Mapping::velocity ( int n ) const
{
    switch ( _type )
    {
        case INSTRUMENT:
            return _instrument->velocity( n );
        case SCALE:
            return 0;
    }

    return 0;
}

const char *
Mapping::note_name ( int n ) const
{
    switch ( _type )
    {
        case INSTRUMENT:
            return _instrument->note_name( n );
        case SCALE:
            return _scale->note_name( _key, n );
    }

    return NULL;
}

/* For the instrument editor... */
Instrument *
Mapping::instrument ( void ) const
{
    if ( IS_INSTRUMENT )
        return _instrument;
    else
        return NULL;
}

/** return a string describing the type of this mapping */
const char *
Mapping::type ( void ) const
{
    return IS_INSTRUMENT ? "Instrument" : "Scale";
}


bool
Mapping::editable ( void ) const
{
    return IS_INSTRUMENT ? true : false;
}
