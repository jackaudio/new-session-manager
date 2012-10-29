
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

#include "scale.H"
#include "common.h"

#include "stdlib.h"
#include <MIDI/midievent.H>

using namespace MIDI;

/* Define some scales. These don't really need to be stored on
   disk. Scales don't change that often. */

const char *Scale::chromatic_names[] =
/* { "A", */
/*   "A#/Bb", */
/*   "B", */
/*   "C", */
/*   "C#/Db", */
/*   "D", */
/*   "D#/Eb", */
/*   "E", */
/*   "F", */
/*   "F#/Gb", */
/*   "G", */
/*   "G#/Ab" }; */

/* { "C", */
/*   "C#/Db", */
/*   "D", */
/*   "D#/Eb", */
/*   "E", */
/*   "F", */
/*   "F#/Gb", */
/*   "G", */
/*   "G#/Ab", */
/*   "A", */
/*   "A#/Bb", */
/*   "B" }; */


{ "C",
  "C#",
  "D",
  "D#",
  "E",
  "F",
  "F#",
  "G",
  "G#",
  "A",
  "A#",
  "B" };


Scale Scale::builtin_scales[] = {
    { "Major",
      7,
      { 0, 2, 4, 5, 7, 9, 11 } },
    { "Natural Minor",
      7,
      { 0, 2, 3, 5, 7, 8, 10 } },
    { "Harmonic Minor",
      7,
      { 0, 2, 3, 5, 7, 8, 11 } },
    { "Melodic Minor",
      7,
      { 0, 2, 3, 5, 7, 9, 11 } },
    { "Major Pentatonic",
      5,
      { 0, 2, 4, 7, 9 } },
    { "Minor Pentatonic",
      5,
      { 0, 3, 5, 7, 10 } },
    { "Chromatic",
      12,
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } }
};

/* const Scale builtin_chords[] = { */
/*     { "Major Triad", */
/*       { 0, 4, 7, -1 } }, */
/*     { "Major Sixth", */
/*       { 0, 4, 7, 9, -1 } }, */
/*     { "Major Seventh", */
/*       { 0, 4, 7, 11, -1 } }, */
/*     { "Major Ninth", */
/*       { 0, 4, 7, 11, 14, -1 } }, */
/*     { "Major 6/9", */
/*       { 0, 4, 7, 9, 14, -1 } }, */
/*     { "Major Eleventh", */
/*       { 0, 4, 7, 11, 14, 17, -1 } }, */
/*     { "Major Thirteenth", */
/*       { 0, 4, 7, 11, 14, 17, 21, -1 } }, */

/*     { "Minor Triad", */
/*       { 0, 3, 7, -1 } }, */
/*     { "Minor Sixth", */
/*       { 0, 3, 7, 9, -1 } }, */
/*     { "Minor Seventh", */
/*       { 0, 3, 7, 10, -1 } }, */
/*     { "Minor Ninth", */
/*       { 0, 3, 7, 10, 14, -1 } }, */
/*     { "Minor 6/9", */
/*       { 0, 3, 7, 9, 14, -1 } }, */
/*     { "Minor Eleventh", */
/*       { 0, 3, 7, 10, 14, 17, -1 } }, */
/*     { "Minor Thirteenth", */
/*       { 0, 3, 7, 10, 14, 17, 21, -1 } }, */

/*     { "Augmented Triad", */
/*       { 0, 4, 8, -1 } }, */
/*     { "Diminished Triad", */
/*       { 0, 3, 6, -1 } } */
/* }; */


/* "Opens" a scale for compatibility with Instrument class */
Scale *
Scale::open ( const char *name )
{
    for ( int i = elementsof( builtin_scales ) - 1; i >= 0; --i )
        if ( 0 == strcmp( name, builtin_scales[i]._name ) )
            return &builtin_scales[i];

    return NULL;
}

/* Returns a list of available scales */
char **
Scale::listing ( void )
{
    char **sa;

    int n = elementsof( builtin_scales );

    sa = (char **)malloc( sizeof ( char * ) * ( n + 1 ) );
    sa[n] = NULL;

    while ( n-- )
        sa[n] = strdup( builtin_scales[n]._name );

    return sa;
}

const char *
Scale::chromatic_name ( int n )
{
    return chromatic_names[ n % 12 ];
}

int
Scale::octave ( int n )
{
    return n / 12;
}

int
Scale::_degree ( int k, int n ) const
{
    return k + _degrees[ n % _notes ];
}

/* translate NOTE event. Behavior is undefined for other event types */
bool
Scale::translate ( int k, midievent *e ) const
{
    if ( ! note_name( k, e->note() ) )
        return false;
    else
        return true;
}

const char *
Scale::note_name ( int k, int n ) const
{
    /* all the magic is here */
    static char s[5];

    const int mod_n = n % 12;

    // FIXME: searching is not efficient!
    for ( int i = _notes; i-- ; )
        if ( mod_n == (_degrees[ i ] + k) % 12 )
        {
            snprintf( s, sizeof(s), "%s%i", 
                      chromatic_names[ mod_n ],
                      n / 12 );

            return s;
        }

    return NULL;
}

const char *
Scale::name ( void ) const
{
    return _name;
}
