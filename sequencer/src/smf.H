
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

#include "grid.H"

class pattern;
class phrase;

#include <stdio.h>

class smf
{

    char * _name;

    int _format;                                                /* 0, 1, 2 */

    FILE *_fp;

    /* reader */
    long _length;                                               /* length of the current chunk  */
    long _pos;                                                  /* number of bytes read from chunk */
    int _ppqn;                                                  /* PPQN of imported files */

    /* writer */
    unsigned int _tally;                                        /* number of bytes written thus far */
    long _num_tracks_pos;                                       /* where to write the number of tracks when known */
    long _length_pos;                                           /* where to write the chunk length when known */
    int _cue;                                                   /* transform note ons to cue events for this track */

    int _tracks;                                                /* number of tracks */
    int _track;                                                 /* current track */
    tick_t _time;                                               /* current timestamp in writer */

    int _mode;

    byte_t _status;

public:

    enum { WRITE, READ };

    unsigned long read_long ( void );
    unsigned short read_short ( void );
    unsigned long read_var ( void );
    void read_bytes ( void *p, int l );
    byte_t read_byte ( void );

    void write_var ( long var );
    void write_long ( unsigned long x );
    void write_ascii ( const char *buf );
    void write_short ( unsigned short x );
    void write_byte ( byte_t b );
    void write_bytes ( const void *p, size_t l );

/* Meta Event codes */
    enum {
        SEQUENCE    = 0x00,
        TEXT        = 0x01,
        COPYRIGHT   = 0x02,
        NAME        = 0x03,
        INSTRUMENT  = 0x04,
        LYRIC       = 0x05,
        MARKER      = 0x06,
        CUEPOINT    = 0x07,
        PROGRAM     = 0x08,
        DEVICE      = 0x09,
        CHANNEL     = 0x20,
        PORT        = 0x21,
        END         = 0x2F,
        TEMPO       = 0x51,
        SMPTE       = 0x54,
        TIMESIG     = 0x58,
        KEYSIG      = 0x59,
        PROPRIETARY = 0x7F
    };

    smf( void );
    ~smf( void  );

    int open ( const char *name, int mode );

    static void print_track_listing ( const char *name );

    void write_meta_event ( byte_t type, int n );
    void write_meta_event ( byte_t type, const char *s );

    void write_event ( const MIDI::midievent *e );
    void write_header ( int tracks );

    void open_chunk ( const char *id );
    void close_chunk ( void );
    void open_track ( const char *name, int num );
    void close_track ( tick_t length );

    void write_pattern_info ( const pattern *p );

    void cue ( bool b );

    list <MIDI::midievent> * read_track_events ( tick_t *length );

    void write_phrase_info ( const phrase *p );

    bool read_song_info( int *mode, int *phrases, int *patterns, char **name, char **notes );
    void write_song_info( int mode, int phrases, int patterns, const char *name, const char *notes );

    void home ( void );
    void skip ( size_t l );
    void backup ( size_t l );

    int next_track ( void );
    bool seek_track ( int n );
    char ** track_listing ( void );

    char * read_cue_point ( void );
    int read_header ( void );
    char * read_text ( void );
    char * read_track_name ( void );
    bool read_phrase_info ( phrase *p );
    bool read_pattern_info ( pattern *p );

    int format ( void ) const;
    int tracks ( void ) const;

};
