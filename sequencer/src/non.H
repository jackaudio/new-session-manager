
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

#include "common.h"
#include "pattern.H"
#include "phrase.H"
#include "sequence.H"

enum {
  PLAY,
  MUTE,
  SOLO
};

class Canvas;
class Lash;

extern Canvas *pattern_c, *phrase_c;
extern sequence *playlist;
extern Lash lash;

void quit ( void );
void init_song ( void );
void handle_midi_input ( void );
bool load_song ( const char *name );
bool save_song ( const char *name );
void setup_jack ( void );

#include "common.h"
#include "const.h"


enum play_mode_e {
  PATTERN,
  SEQUENCE,
  TRIGGER,
  QUEUE
//  PHRASE,
};


enum record_mode_e {
    MERGE,
    OVERWRITE,
    LAYER,
    NEW
};


/* program settings (from rc file) */
struct global_settings {

    enum record_mode_e record_mode;

    bool record_filtered;                                       /* ignore non-note events while recording */
    bool visual_metronome;                                      /* show visual metronome */
    bool follow_playhead;

    char *user_config_dir;

};
extern global_settings config;

/* song settings (from song file) */
struct song_settings
{

    enum play_mode_e play_mode;

    char *filename;

    signal <void> signal_dirty;                                 /* emitted when first dirtied */
    signal <void> signal_clean;                                 /* emitted when first cleaned */

    bool _dirty;

    bool dirty ( void )
        {
            return _dirty;
        }

    void
    dirty( bool b )
        {
            if ( _dirty != b )
            {
                _dirty = b;

                if ( b )
                {
                    DMESSAGE( "song is now dirty" );
                    signal_dirty();
                }
                else
                {
                    DMESSAGE( "song is now clean" );
                    signal_clean();
                }
            }
        }

    void
    set_dirty ( void )
        {
            dirty( true );
        }

};
extern song_settings song;
