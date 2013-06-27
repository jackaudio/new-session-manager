
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

#include "midievent.H"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"

namespace MIDI
{
    static const char *opcode_names[] =
    {
        "Note Off",
        "Note On",
        "Aftertouch",
        "Control Change",
        "Program Change",
        "Channel Pressure",
        "Pitch Wheel"
    };

    midievent::midievent ( void )
    {
        _sysex = NULL;
        _timestamp = 0;
        _data.status = NOTE_OFF;
        _data.msb =  _data.lsb = 0;
    }

    midievent::~midievent ( void )
    {
        if ( _sysex )
            delete _sysex;

        _sysex = NULL;
    }

    int
    midievent::pitch ( void ) const
    {
        return ((_data.msb << 7) | _data.lsb) - 0x2000;
    }

    void
    midievent::pitch ( int n )
    {
        n += 0x2000;

        _data.lsb = n & 0x7F;
        _data.msb = (n >> 7) & 0x7F;
    }

    void
    midievent::data ( byte_t D1, byte_t D2 )
    {
        _data.lsb = D1 & 0x7F;
        _data.msb = D2 & 0x7F;
    }

    void
    midievent::data ( byte_t *D1, byte_t *D2 ) const
    {
        *D1 = _data.lsb;
        *D2 = _data.msb;
    }

    void
    midievent::raw ( byte_t *p, int l) const
    {
        memcpy( p, &_data, l );
    }

    int
    midievent::size ( void ) const
    {
        return midievent::event_size( opcode() );
    }

    void
    midievent::note_velocity ( int vel )
    {
        _data.msb = vel & 0x7F;
    }

    unsigned char
    midievent::note ( void ) const
    {
        return _data.lsb;
    }

    void
    midievent::note ( char note )
    {
        _data.lsb = note & 0x7F;
    }

    unsigned char
    midievent::note_velocity ( void ) const
    {
        return _data.msb;
    }

    bool
    midievent::is_same_note ( midievent * e ) const
    {
        return channel() == e->channel() && note() == e->note();
    }

/** get name from opcode */
    const char *
    midievent::name ( void ) const
    {
        return opcode_names[ (opcode() >> 4) - 8 ];
    }

/** get opcode from name */
    int
    midievent::name ( const char *name ) const
    {
        for ( unsigned int i = elementsof( opcode_names ); i--; )
            if ( ! strcmp( name, opcode_names[ i ] ) )
                return (i + 8) << 4;

        return -1;
    }

/** print event in hexadecimal */
    void
    midievent::print ( void ) const
    {
        printf( "[%06f] %02X %02X %02X\n",
                _timestamp,
                _data.status,
                _data.lsb,
                _data.msb );
    }

/** print event in english/decimal */
    void
    midievent::pretty_print ( void ) const
    {
        printf(
            "[%06f] %-15s c: %2d d1: %3d d2: %3d\n",
            _timestamp,
            name(),
            channel(),
            _data.lsb,
            _data.msb );
    }


/*********/
/* Sysex */
/*********/

    midievent::sysex::sysex ( void )
    {
        _data = NULL;
        _size = 0;
        _alloc = 0;
    }

    midievent::sysex::~sysex ( void )
    {
        if ( _data )
            free( _data );

        _data = NULL;
    }

/** add bytes to sysex message  */
    void
    midievent::sysex::append ( byte_t *data, size_t size )
    {
        if ( _size + size > _alloc )
            _data = (byte_t *)realloc( _data, _alloc += 256 );

        memcpy( data + _size, data, size );

        _size += size;
    }

/** return SysEx data */
    const byte_t *
    midievent::sysex::data ( void ) const
    {
        return _data;
    }

    long
    midievent::sysex::size ( void ) const
    {
        return _size;
    }



    bool
    midievent::operator== ( const midievent &rhs ) const
    {
        return _timestamp == rhs._timestamp &&
            ! bcmp( (void*)&_data, (void*)&rhs._data, size() );
    }
}
