
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

/* this class represents a single raw MIDI event plus a timestamp */

#pragma once

#include "types.h"
#include <stdlib.h>

namespace MIDI
{
    class midievent
    {

    public:

        class sysex {
            size_t _size, _alloc;
            byte_t *_data;

        public:

            sysex ( void );
            ~sysex ( void );

            void append ( byte_t *data, size_t size );
            const byte_t * data ( void ) const;
            long size ( void ) const;

        };

    private:

        sysex *_sysex;

        tick_t _timestamp;                                          /* in ticks */
        struct {
            byte_t status,                                          /* full status byte */
                lsb,                                                /* data 1 */
                msb;                                                /* data 2 */
        } _data;

    public:

        static inline int
        event_size ( byte_t op )
            {
                switch ( op )
                {
                    case NOTE_ON:        case NOTE_OFF: case AFTERTOUCH:
                    case CONTROL_CHANGE: case PITCH_WHEEL:
                        return 3;
                    case PROGRAM_CHANGE: case CHANNEL_PRESSURE:
                        return 2;
                    default:
                        return 1;
                }
            };

        /* define MIDI status bytes */
        enum {
            STATUS_BIT       = 0x80,
            NOTE_OFF         = 0x80,
            NOTE_ON          = 0x90,
            AFTERTOUCH       = 0xA0,
            CONTROL_CHANGE   = 0xB0,
            PROGRAM_CHANGE   = 0xC0,
            CHANNEL_PRESSURE = 0xD0,
            PITCH_WHEEL      = 0xE0,
            CLEAR_CHAN_MASK  = 0xF0,
            MIDI_CLOCK       = 0xF8,
            SYSEX            = 0xF0,
            SYSEX_END        = 0xF7,
            META             = 0xFF
        };

        midievent ( void );
        virtual ~midievent ( void );

        tick_t timestamp ( void ) const;
        void timestamp ( tick_t time );
        void status ( byte_t status );
        byte_t status ( void ) const;
        void channel ( byte_t channel );
        byte_t channel ( void ) const;
        byte_t opcode ( void ) const;
        void opcode ( byte_t o );
        void lsb ( byte_t n );
        void msb ( byte_t n );
        int lsb ( void ) const;
        int msb ( void ) const;
        int pitch ( void ) const;
        void pitch ( int n );
        void data ( byte_t D1, byte_t D2 );
        void data ( byte_t *D1, byte_t *D2 ) const;
        void raw ( byte_t *p, int l) const;
        int size ( void ) const;
        void note_velocity ( int vel );
        bool is_note_on ( void ) const;
        bool is_note_off ( void ) const;
        virtual unsigned char note ( void ) const;
        virtual void note ( char note );
        unsigned char note_velocity ( void ) const;
        bool is_same_note ( midievent * e ) const;
        const char * name ( void ) const;
        int name ( const char *name ) const;
        void print ( void ) const;
        void pretty_print ( void ) const;

        bool operator< ( const midievent &rhs ) const;
        bool operator>= ( const midievent &rhs ) const;

        bool operator== ( const midievent &rhs ) const;

    };


/**********************/
/* Inlined accessors  */
/**********************/


    inline tick_t
    midievent::timestamp ( void ) const
    {
        return _timestamp;
    }

    inline void
    midievent::timestamp ( tick_t time )
    {
        _timestamp = time;
    }

    inline void
    midievent::status ( byte_t status )
    {
        _data.status = status;
    }

    inline byte_t
    midievent::status ( void ) const
    {
        return _data.status;
    }

    inline void
    midievent::channel ( byte_t channel )
    {
        _data.status = (_data.status & 0xF0) | (channel & 0x0F);
    }

    inline byte_t
    midievent::channel ( void ) const
    {
        return _data.status & 0x0F;
    }

    inline byte_t
    midievent::opcode ( void ) const
    {
        return _data.status & 0xF0;
    }


    inline void
    midievent::opcode ( byte_t opcode )
    {
        _data.status = (_data.status & 0x0F) | (opcode & 0xF0);
    }

    inline void
    midievent::lsb ( byte_t n )
    {
        _data.lsb = n & 0x7F;
    }

    inline void
    midievent::msb ( byte_t n )
    {
        _data.msb = n & 0x7F;
    }

    inline int
    midievent::lsb ( void ) const
    {
        return _data.lsb;
    }

    inline int
    midievent::msb ( void ) const
    {
        return _data.msb;
    }

    inline bool
    midievent::is_note_on ( void ) const
    {
        return (opcode() == NOTE_ON);
    }

    inline bool
    midievent::is_note_off ( void ) const
    {
        return (opcode() == NOTE_OFF);
    }

    inline bool
    midievent::operator< ( const midievent &rhs ) const
    {
        return _timestamp < rhs._timestamp;
    }

    inline bool
    midievent::operator>= ( const midievent &rhs ) const
    {
        return _timestamp >= rhs._timestamp;
    }

}
