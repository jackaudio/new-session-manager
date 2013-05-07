
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "instrument.H"
#include "common.h"
#include "const.h"
#include "config.h"

#include "non.H"

#include <fnmatch.h>
#include <dirent.h>


#include <list>

#include <string>

#include <MIDI/event.H>
using namespace MIDI;

using std::list;
using std::string;

/******
       Instrument definition file format is thus:

       "Name", n, v

       Where /n/ is a note number from 0 to 127 and /v/ is a percentage of
       volume.

       When a system installed instrument definition is modified, the
       modified version is saved in the user's $HOME. Therefore, when
       loading instruments, user defined instruments always hide system
       defined instruments of the same name.

*/

list <Instrument *> Instrument::instruments;

Instrument::Instrument ( const char *name )
{

    for ( int i = 0; i < 128; i++ )
    {
        _map[i].name = NULL;
        _map[i].velocity = 100;
    }

    if ( name )
    {
        _name = strdup( name );

        read( name );
    }
    else
    {
        _name = strdup( "not an instrument" );
        _height = 0;
    }

    Instrument::instruments.push_front( this );
}

Instrument *
Instrument::open ( const char *name )
{
    list <Instrument *>::iterator i = Instrument::instruments.begin();

    if ( name )
    {
        for ( ; i != Instrument::instruments.end(); i++ )
            if ( 0 == strcmp( (*i)->_name, name ) )
                return *i;
    }

    return new Instrument ( name );
}

void
Instrument::note_name ( int n, char *s )
{
    if ( _map[ n ].name )
        free( _map[ n ].name );

    _map[ n ].name = s;

    _dirty = true;
}

void
Instrument::velocity ( int n, int v )
{
    _map[ n ].velocity = v;

    _dirty = true;
}

/** Translate event, should only be passed NOTE ON/OFF events, returns
    true if note is valid for this mapping */
bool
Instrument::translate ( midievent *e ) const
{
    if ( ! note_name( e->note() ) )
        return false;

    e->note_velocity( e->note_velocity() * _map[ e->note() ].velocity / 100 );

    return true;
}

const char *
Instrument::note_name ( int n ) const
{
    return _map[ n ].name;
}

int
Instrument::height ( void ) const
{
    return _height;
}

int
Instrument::velocity ( int n ) const
{
    return _map[ n ].velocity;
}

bool
Instrument::read ( const char *s )
{
    FILE *fp;

    char pat[512];

    sprintf( pat, "%s%s.inst", config.user_config_dir, s );

    if ( ! ( fp = fopen( pat, "r" ) ) )
    {

        sprintf( pat, "%s/%s/%s.inst", SYSTEM_PATH, INSTRUMENT_DIR, s );

        if ( ! ( fp = fopen( pat, "r" ) ) )
            return false;
    }

    struct i_map m;
    char namebuf[256];
    int note, velocity;

    int n = 0;
    while ( 0 < fscanf( fp, "\"%[^\"]\", %d, %d\n", (char*)&namebuf, &note, &velocity ) ) n++;

    rewind( fp );

    MESSAGE( "reading %d lines from instrument file \"%s\"", n, s );

    int i;
    for ( i = 0; i < n; i++ )
    {
        fscanf( fp, "\"%[^\"]\", %d, %d\n", (char *)&namebuf, &note, &velocity );

        m.name = strdup( namebuf );

        if ( velocity > 100 )
        {
            WARNING( "invalid volume percentage in instrument definition");
            m.velocity = 100;
        }
        else
            m.velocity = velocity;

        DMESSAGE( "name: \"%s\", note: %d, velocity: %d%%", m.name, note, m.velocity );

        _map[ note ] = m;
    }

    _height = n;

    fclose( fp );

    return true;
}

bool
Instrument::write ( const char *s ) const
{
    FILE *fp;

    char pat[512];

    sprintf( pat, "%s/%s.inst", config.user_config_dir, s );

    if ( ! ( fp = fopen( pat, "w" ) ) )
        return false;

    int n = 0;
    for ( int i = 0; i < 127; ++i )
    {
        if ( _map[ i ].name )
        {
            fprintf( fp, "\"%s\", %d, %d\n", _map[ i ].name, i, _map[ i ].velocity );
            ++n;
        }
    }

    DMESSAGE( "wrote %d lines to instrument file \"%s\"", n, pat );

    fclose( fp );

    return true;
}

void
Instrument::save ( void ) const
{
    if ( _dirty )
        write( _name );

    _dirty = false;
}

static int
instrument_filter ( const struct dirent *d )
{
    char suffix[] = "*.inst";

    return 0 == fnmatch( suffix, d->d_name, 0 );
}

static
list <string> *
get_listing( const char *dir )
{
    list <string> *sl = new list <string>;

    struct dirent **names;
    int n;

    if ( 0 > ( n = scandir( dir, &names, instrument_filter, alphasort ) ) )
    {
        WARNING( "couldn't open instrument directory" );
        return NULL;
    }
    else
    {
        while (n--)
        {
            char *c = rindex( names[n]->d_name, '.' );

            if ( c )
                *c = '\0';

            MESSAGE( "found instrument: %s", names[n]->d_name );

            string s( names[n]->d_name );

            sl->push_back( s );

            free( names[n] );
        }
        free( names );

        return sl;
    }
}

/* Returns a list of available instruments */
char **
Instrument::listing ( void )
{
    list <string> *sys = get_listing( SYSTEM_PATH "/" INSTRUMENT_DIR );
    list <string> *usr = get_listing( config.user_config_dir );

    if ( ! ( usr || sys ) )
        return NULL;

    if ( sys && usr )
        usr->merge( *sys );
    else
        if ( sys && ! usr )
            usr = sys;

    usr->unique();

    usr->sort();

    if ( sys )
        delete sys;

    char **sa = (char**)malloc( (usr->size() + 1) * sizeof( char * ) );

    int i = 0;
    for ( list <string>::iterator s = usr->begin(); s != usr->end(); s++, i++ )
        sa[i] = strdup( s->c_str() );

    sa[i] = NULL;

    delete usr;

    return sa;
}

const char *
Instrument::name ( void ) const
{
    return _name;
}
