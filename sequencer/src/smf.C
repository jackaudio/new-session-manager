
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

#include "smf.H"
#include "phrase.H"
#include "pattern.H"

using namespace MIDI;

smf::smf ( void )
{
    _name = NULL;
    _pos = 0;

    _fp = NULL;

    _length = 0;
    _length_pos = 0;
    _num_tracks_pos = 0;
    _tracks = 0;
    _time = 0;
    _tally = 0;
    _cue = 0;
    _track = 0;
}

smf::~smf ( void )
{
    /* fill in the number of tracks */
    if ( _num_tracks_pos )
    {
        fseek( _fp,  _num_tracks_pos, SEEK_SET );
        write_short( _tracks );
    }

    if ( _fp )
        fclose( _fp );

    if ( _name )
        free( _name );
}

int
smf::open ( const char *name, int mode )
{
    _name = strdup( name );

    _mode = mode;

    _fp = fopen( _name, mode == smf::WRITE ? "w" : "r" );

    return _fp != NULL;
}

/*************************/
/* Private bit twiddlers */
/*************************/

unsigned long
smf::read_long ( void )
{
    byte_t buf[4];
    unsigned long ret = 0;

    read_bytes( buf, 4 );

    ret += *(buf + 0) << 24;
    ret += *(buf + 1) << 16;
    ret += *(buf + 2) << 8;
    ret += *(buf + 3);

    return ret;
}

unsigned short
smf::read_short ( void )
{
    byte_t buf[2];
    unsigned short ret = 0;

    read_bytes( buf, 2 );

    ret += *(buf + 0) << 8;
    ret += *(buf + 1);

    return ret;
}

unsigned long
smf::read_var ( void )
{
    unsigned long ret = 0;
    unsigned char c;

    /* while bit #7 is set */
    while ( ( ( c = read_byte() ) & 0x80 ) != 0x00 )
    {

        /* shift ret 7 bits */
        ret <<= 7;
        /* add bits 0-6 */
        ret += c & 0x7F;
    }

    /* bit was clear */
    ret <<= 7;
    ret += c & 0x7F;

    return ret;
}

void
smf::read_bytes ( void *p, int l )
{
    fread( p, l, 1, _fp );

    _pos += l;
}

byte_t
smf::read_byte ( void )
{
    byte_t b;
    read_bytes( &b, 1 );

    return b;
}

void
smf::write_var ( long var )
{
    long buffer;
    buffer = var & 0x7F;

    /* we shift it right 7, if there is
       still set bits, encode into buffer
       in reverse order */
    while ( ( var >>= 7) )
    {
	buffer <<= 8;
	buffer |= ( var & 0x7F ) | 0x80;
    }

    for ( ;; )
    {
        write_byte( buffer );

        if ( buffer & 0x80 )
            buffer >>= 8;
        else
            break;
    }
}


void
smf::write_long ( unsigned long x )
{
    byte_t buf[4];

    buf[0] = ( x & 0xFF000000 ) >> 24;
    buf[1] = ( x & 0x00FF0000 ) >> 16;
    buf[2] = ( x & 0x0000FF00 ) >> 8;
    buf[3] = x & 0x000000FF;

    write_bytes( buf, 4 );
}

void
smf::write_ascii ( const char *buf )
{
    if ( strlen( buf ) != 4 )
        ASSERTION( "invalid MIDI value" );

    write_bytes( (void *)buf, 4 );
}

void
smf::write_short ( unsigned short x )
{
    byte_t buf[2];

    buf[0] = (x & 0xFF00 ) >> 8;
    buf[1] = x & 0x00FF;

    write_bytes( buf, 2 );
}

void
smf::write_byte ( byte_t b )
{
    write_bytes( &b, 1 );
}


void
smf::write_bytes ( const void *p, size_t l )
{
    fwrite( p, l, 1, _fp );
    _tally += l;
}



/*************************/
/* Read and write tracks */
/*************************/

/* write event /e/ to the currently open file (should only be used in a track)
   if /cue/ is true, transform a notes-on/off into cue messages */
void
smf::write_event ( const midievent *e )
{
    tick_t ts = e->timestamp();
    tick_t delta = ts - _time;
    _time = ts;

    write_var( delta );

    if ( _cue && (e->is_note_off() || e->is_note_on() ) )
    {
        /* begin cue message */
        write_byte( 0xF0 );                                    /* sysex */

        write_var( 7 );                                        /* length of this message */

        static byte_t data[] = { 0x7F,                         /* MTC */
                                 0,                            /* id */
                                 0x05 };                       /* cue message */

        write_bytes( data, sizeof( data ) );

        write_byte( e->opcode() == event::NOTE_ON ? 0x05 : 0x06 );
        write_short( e->note() );

        /* terminate */
        write_byte( 0xF7 );

        _status = 0;
    }
    else
    {
        byte_t buf[4];

        int l = e->size();

        midievent me = *e;

        if ( me.opcode() == event::NOTE_OFF )
        {
            me.opcode( event::NOTE_ON );
            me.note_velocity( 0 );
        }

        me.raw( buf, l );

        /* write with running status */
        if ( buf[0] != _status )
        {
            write_bytes( buf, l );
            _status = buf[0];
        }
        else
            write_bytes( buf + 1, l - 1 );

    }
}

void
smf::write_header ( int fmt )
{
    write_ascii( "MThd" );
    write_long( 6 );                                             /* Always 6 bytes of header */

    _format = fmt;

    write_short( fmt );                                          /* format, SMF-0 for 1 track SMF-2 for more */

    _num_tracks_pos = ftell( _fp );
    _tracks = 0;

    write_short( 0xDEAF );

    write_short( PPQN );
}

/* start a new MIDI 'chunk', /id/ is 4 letters of ASCII */
void
smf::open_chunk ( const char *id )
{
    if ( _length_pos )
        ASSERTION( "chunks cannot be nested!" );

    write_ascii( id );

    /* reset track length counter */
    _length_pos = ftell( _fp );

    write_long( 0xBEEFCAFE );                                   /* length, this has to be filled in at track end! */

    _tally = 0;
    _time = 0;
}

void
smf::close_chunk ( void )
{
    /* fill in track length  */
    long here = ftell( _fp );

    fseek( _fp, _length_pos, SEEK_SET );

    write_long( _tally );

    fseek( _fp, here, SEEK_SET );

    /* cleanup */
    _length_pos = 0;
    _tally = 0;
}

void
smf::open_track ( const char *name, int num )
{
    open_chunk( "MTrk" );

    if ( _format == 2 && num >= 0 )
        write_meta_event ( smf::SEQUENCE, num );

    if ( name )
        write_meta_event ( smf::NAME, name );

    ++_tracks;

    _status = 0;

    // FIXME: write time signature here
}

void
smf::close_track ( tick_t length )
{
    /* end */
    write_meta_event( smf::END, length ? length - _time : 0 );

    _cue = 0;

    close_chunk();
}

void
smf::write_meta_event ( byte_t type, int n )
{
    write_var( type == smf::END ? n : 0 );                                             /* delta time */
    write_short( 0xFF00 + type );

    /* write length bytes */
    switch ( type )
    {
        case smf::TEMPO:
            write_byte( 3 );
// FIXME:
            break;
        case smf::SEQUENCE:
            write_byte( 2 );
            write_short( n );
            break;
        case smf::CHANNEL:
        case smf::PORT:
            write_byte( 1 );
            write_byte( n );
            break;
        case smf::END:
            write_byte( 0x00 );
            break;
        case smf::PROPRIETARY:
            // length
            write_var( n );
            break;
// FIXME: handle time sig, key sig, proprietary
    }

    _status = 0;
}

void
smf::write_meta_event ( byte_t type, const char *s )
{
    write_var( 0 );
    write_short( 0xFF00 + type );

    switch ( type )
    {
        case smf::TEXT:
        case smf::NAME:
        case smf::INSTRUMENT:
        case smf::COPYRIGHT:
        case smf::LYRIC:
        case smf::MARKER:
        case smf::CUEPOINT:
        {
            int l = strlen( s );

            write_var( l );

            write_bytes( s, l );

            break;
        }
        default:
            ASSERTION( "event type does not take text!" );
            break;
    }
}

/** write song gloabl info (only used on playlist track) */
void
smf::write_song_info ( int mode, int phrases, int patterns, const char *name, const char *notes )
{
    write_meta_event( smf::PROPRIETARY, 5 + (4 * 2) /* length */ );

    write_ascii( "Non!" );

    write_byte( mode );
    write_long( phrases );
    write_long( patterns );

    if ( name )
        write_meta_event( smf::NAME, name );

    write_meta_event( smf::TEXT, ":: Created by the Non-Seqeuncer" );

    if ( notes )
        write_meta_event( smf::TEXT, notes );
}

void
smf::write_phrase_info ( const phrase *p )
{
    if ( p->notes() )
        write_meta_event( smf::TEXT, p->notes() );

    char *s = p->viewport.dump();

    char pat[156];
    snprintf( pat, sizeof( pat ), "Non: xywh=%s",
              s );

    free( s );

    write_meta_event( smf::PROPRIETARY, strlen( pat ) );
    write_bytes( pat, strlen( pat ) );
}

/** write proprietary pattern info meta event */
void
smf::write_pattern_info ( const pattern *p )
{
    write_meta_event( smf::PORT, p->port() );

    char pat[256];

    snprintf( pat, sizeof( pat ), "%s: %s", p->mapping.type(), p->mapping.name() );

    write_meta_event( smf::INSTRUMENT, pat );

    if ( p->notes() )
        write_meta_event( smf::TEXT, p->notes() );

    char *s = p->viewport.dump();

    snprintf( pat, sizeof( pat ), "Non: xywh=%s, ppqn=%d, key=%d, note=%d, mode=%d",
              s, p->ppqn(), p->mapping.key(), p->note(), p->mode() );

    free( s );

    write_meta_event( smf::PROPRIETARY, strlen( pat ) );
    write_bytes( pat, strlen( pat ) );
}

/* turn on note->cue translation for this track */
void
smf::cue ( bool b )
{
    _cue = b;
}


/**********/
/* Reader */
/**********/

char *
smf::read_text ( void )
{
    int l = read_var();

    char *s = (char*) malloc( l + 1 );

    read_bytes( s, l );

    s[l] = '\0';

    return s;
}


int
smf::read_header ( void )
{
    char id[4];
    read_bytes( id, 4 );

    if ( strncmp( id, "MThd", 4 ) )
        return 0;

    if ( read_long() != 6 )
        return 0;

    _format = read_short();
    _tracks = read_short();
    _ppqn   = read_short();

    _pos = 0;

    return 1;
}

void
smf::home ( void )
{
    fseek( _fp, 14, SEEK_SET );

    _track = 0;
    _pos = 0;
    _length = 0;
}

void
smf::skip ( size_t l )
{
    fseek( _fp, l, SEEK_CUR );
    _pos += l;
}

void
smf::backup ( size_t l )
{
    skip( 0 - l );
}

char *
smf::read_track_name ( void )
{
    int status;
    long where = 0;
    int num = 0;

    for ( num = 0; ; ++num )
    {
        where = _pos;

        read_var();                                              /* delta */
        status = read_byte();

        /* stop at first non meta-event */
        if ( status != midievent::META )
            break;

        int opcode = read_byte();

        switch ( opcode )
        {
            case smf::NAME:
                return read_text();
            case smf::TEXT:
                return read_text();
            default:
                skip( read_var() );
        }
    }

    backup( _pos - where );

    return NULL;
}

/** read next Cue Point event on track */
char *
smf::read_cue_point ( void )
{
    read_var();                                                 /* delta */

    int status = read_byte();

    if ( status != midievent::META )
        return NULL;

    int opcode = read_byte();

    if ( opcode != smf::CUEPOINT )
        return NULL;

    return read_text();
}

bool
smf::read_song_info ( int * mode, int * phrases, int *patterns, char **name,  char **notes )
{
    int status;
    long where = 0;
    int num = 0;
    bool r = false;
    *notes = NULL;

    for ( num = 0; ; ++num )
    {
        where = _pos;

        read_var();                                              /* delta */
        status = read_byte();

        /* stop at first non meta-event */
        if ( status != midievent::META )
            break;

        int opcode = read_byte();

        switch ( opcode )
        {
            case smf::PROPRIETARY:
            {
                int len = read_var();

                if ( len < 5 + (2 * 4) )
                    return false;

                char id[4];
                read_bytes( id, 4 );

                if ( strncmp( id, "Non!", 4 ) )
                    return false;

                *mode = read_byte();
                *phrases = read_long();
                *patterns = read_long();

                r = true;

                break;
            }
            case smf::TEXT:
            {
                char *text = read_text();

                if ( ! strncmp( text, "::", 2 ) )
                    free( text );
                else
                    *notes = text;

                break;
            }
            case smf::NAME:
                *name = read_text();
                break;
            case smf::END:
                goto done;
            default:
                goto semidone;
        }
    }

semidone:

    backup( _pos - where );

done:

    return r;
}

bool
smf::read_phrase_info ( phrase *p )
{
    int status;
    long where = 0;
    int num = 0;

    for ( num = 0; ; ++num )
    {
        where = _pos;

        read_var();                                              /* delta */
        status = read_byte();

        /* stop at first non meta-event */
        if ( status != midievent::META )
            break;

        int opcode = read_byte();

        switch ( opcode )
        {
            case smf::SEQUENCE:
                /* currently, this is ignored */
                read_var();
                read_short();
                break;
            case smf::NAME:
                p->name( read_text() );
                DMESSAGE( "Track name: %s", p->name() );
                break;
            case smf::INSTRUMENT:
                skip( read_var() );
                break;
            case smf::TEXT:
                p->notes( read_text() );
                break;
            case smf::PROPRIETARY:
            {
                int l = read_var();

                char *data = (char *) alloca( l ) + 1;

                read_bytes( data, l );

                data[l] = '\0';

                char *s;

                if ( 1 != sscanf( data, "Non: xywh=%m[0-9:]",
                                  &s ) )
                    WARNING( "Invalid phrase info event" );
                else
                {
                    p->viewport.read( s );
                    free( s );
                }
                break;
            }
            case smf::END:
                /* Track ends before any non meta-events... */
                read_byte();
                goto done;
            default:
                int l = read_var();
                skip( l );
                WARNING( "skipping unrecognized meta event %02X", opcode );
                break;
        }
    }

    backup( _pos - where );

done:

    return num ? p : NULL;
}

/** inform pattern /p/ from meta-events at the beginning of the
    current track */
bool
smf::read_pattern_info ( pattern *p )
{
    int status;
    long where = 0;
    int num = 0;

    bool name_set = false;

    for ( num = 0; ; ++num )
    {
        where = _pos;

        read_var();                                              /* delta */
        status = read_byte();

        /* stop at first non meta-event */
        if ( status != midievent::META )
            break;

        int opcode = read_byte();

        switch ( opcode )
        {
            case smf::SEQUENCE:
                /* currently, this is ignored */
                read_var();
                read_short();
                break;
            case smf::NAME:
                p->name( read_text() );
                DMESSAGE( "Track name: %s", p->name() );
                name_set = true;
                break;
            case smf::INSTRUMENT:
            {
                char *s = read_text();

                char pat[256];

                if ( 1 == sscanf( s, "Instrument: %[^\n]", pat ) )
                {
                    if ( ! p->mapping.open( Mapping::INSTRUMENT, pat ) )
                    {
                        p->mapping.open( Mapping::SCALE, "Chromatic" );
                        WARNING( "could not find instrument \"%s\"", pat );
                    }
                }
                else
                    if ( 1 == sscanf( s, "Scale: %[^\n]", pat ) )
                    {
                        if ( ! p->mapping.open( Mapping::SCALE, pat ) )
                        {
                            p->mapping.open( Mapping::SCALE, "Chromatic" );
                            WARNING( "could not find scale \"%s\"", pat );
                        }
                    }
                break;
            }
            case smf::PORT:
                read_byte();
                p->port( read_byte() );
                break;
            case smf::TEXT:
                if ( ! name_set )
                {
                    /* also accept TEXT event as name if no name was
                       provided--this is found in a number of older MIDI
                       files. */
                    p->name( read_text() );
                    name_set = true;
                }
                else
                    p->notes( read_text() );
                break;
            case smf::PROPRIETARY:
            {
                int l = read_var();

                char *data = (char *) alloca( l ) + 1;

                read_bytes( data, l );

                data[l] = '\0';

                int ppqn, key, note, mode;
                char *s;

                if ( 5 != sscanf( data, "Non: xywh=%m[0-9:], ppqn=%d, key=%d, note=%d, mode=%d",
                                  &s, &ppqn, &key, &note, &mode ) )
                    WARNING( "Invalid pattern info event" );
                else
                {
                    p->viewport.read( s );
                    free( s );

                    p->ppqn( ppqn );

                    if ( key > 0 )
                        p->mapping.key( key );

                    p->note( note );
                    p->mode( mode );
                }
                break;
            }
            case smf::END:
                /* Track ends before any non meta-events... */
                read_byte();
                goto done;
            default:
                int l = read_var();
                skip( l );
                WARNING( "skipping unrecognized meta event %02X", opcode );
                break;
        }
    }

    backup( _pos - where );

done:

    return num ? p : NULL;
}

int
smf::next_track ( void )
{
    /* first, skip to the end of the track we're on, if any */
    if ( _length )
        skip( _length - _pos );

    while ( ! feof( _fp ) && _track < _tracks )
    {
        char id[4];
        read_bytes( id, 4 );
        _length = read_long();

        if ( strncmp( id, "MTrk", 4 ) )
        {
            WARNING( "skipping unrecognized chunk \"%s\"", id );
            /* not a track chunk */
            skip( _length );
            continue;
        }

        _pos = 0;
        ++_track;
        return 1;
    }

    return _length = _pos = 0;
}


/** locate track number /n/ */
bool
smf::seek_track ( int n )
{
    home();

    if ( n >= _tracks )
        return false;

    for ( int i = 0; next_track(); ++i )
        if ( i == n )
            break;

    return true;
}

char **
smf::track_listing ( void )
{
    if ( _pos != 0 )
        ASSERTION( "attempt to get track listing while in the middle of reading a track." );

    char **sa = (char**)malloc( sizeof( char* ) * (_tracks + 1) );
    int i;

    long where = ftell( _fp );

    for ( i = 0; next_track(); ++i )
    {
        sa[i] = read_track_name();
        sa[i] = sa[i] ? sa[i] : strdup( "<Unnamed>" );
    }

    sa[i] = NULL;

    /* go back to where we started */
    fseek( _fp, where, SEEK_SET );
    _pos = 0;

    return sa;
}

/* print track list for file /name/ */
void
smf::print_track_listing ( const char *name )
{
    smf f;

    f.open( name, smf::READ );

    f.read_header();

    char **sa = f.track_listing();

    char *s;
    for ( int i = 0; (s = sa[i]); ++i )
        printf( "Track %3d: \"%s\"\n", i, s );
}

/** read all remaining events in current track and return them in a list */
list <midievent> *
smf::read_track_events ( tick_t *length )
{
    list <midievent> *events = new list <midievent>;
    event e;

    *length = 0;

    byte_t oldstatus = -1;
    tick_t time = 0;
    tick_t tick = 0;
    tick_t delta;

    while ( _pos < _length )
    {
        byte_t data[3];

        delta = read_var();

        int status = read_byte();

        if ( ! (status & 0x80) )
        {
            backup( 1 );
            status = oldstatus;
        }
        else
            oldstatus = status;

        time += delta;
        tick = (time * PPQN) / _ppqn;

        e.timestamp( tick );

        int opcode = status & 0xF0;

//        e.status( opcode );
        e.status( status );

        switch ( opcode )
        {
            case event::NOTE_OFF:
            case event::NOTE_ON:
            case event::AFTERTOUCH:
            case event::CONTROL_CHANGE:
            case event::PITCH_WHEEL:

                read_bytes( data, 2 );

                /* handle note off, vel 0 */
                if ( opcode == event::NOTE_ON && 0 == data[1] )
                {
                    e.opcode( event::NOTE_OFF );
                    data[1] = 127;
                }

                e.data( data[0], data[1] );

                events->push_back( e );

                /* TODO: set MIDI channel here */
                break;
            case event::PROGRAM_CHANGE:
            case event::CHANNEL_PRESSURE:

                data[0] = read_byte();

                e.lsb( data[0] );

                events->push_back( e );
                break;
            case 0xF0:
                /* TODO: hanlde proprietary events? */
                if ( midievent::META != status )
                {
                    if ( 0xF0 == status )
                    {
                        /* looks like a sysex */
                        int l = read_var();

                        if ( l < 4 )
                            ASSERTION( "unrecognized message" );

                        byte_t *data = (byte_t *) alloca( 4 );

                        read_bytes( data, 4 );

                        l -= 4;

                        if ( data[0] == 0x7F &&
                             data[2] == 0x05 )
                        {
                            /* looks like a cue message! */

                            switch ( data[3] )
                            {
                                case 0x05:
                                    /* start */
                                    e.status( event::NOTE_ON );
                                    e.note( read_short() );
                                    events->push_back( e );
                                    l -= 2;
                                    break;
                                case 0x06:
                                    /* stop */
                                    e.status( event::NOTE_OFF );
                                    e.note( read_short() );
                                    events->push_back( e );
                                    l -= 2;
                                    break;
                                default:
                                    ASSERTION( "unrecognized cue message" );
                                    break;
                            }
                        }

                        DMESSAGE( "converting MIDI cue to note-on/off n: %d", e.note() );

                        /* just in case */
                        skip( l );
                    }
                    else
                    {
                        WARNING( "unrecognized opcode %02X", status );
                        // FIXME: what now?
                    }
                    break;
                }

                opcode = read_byte();

                switch ( opcode )
                {
                    case smf::END:                                  /* track end */
                        /* track extends until this event */
                        *length = tick;

                        if ( read_byte() )
                            WARNING( "corrupt MIDI file in track end" );
                        goto done;
                        break;
                    default:
                        WARNING( "unhandled meta-event %02X", opcode );
                        skip( read_var() );
                        break;
                }
        }
    }

done:

    return events;
}


/**************************/
/* accessors (for reader) */
/**************************/

int
smf::format ( void ) const
{
    return _format;
}

int
smf::tracks ( void ) const
{
    return _tracks;
}
