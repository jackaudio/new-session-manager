
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

#include "Sequence.H"
#include "Region.H"
#include "Track.H"

#include <FL/Fl_Input.H>

class Audio_Sequence : public Sequence
{

protected:

    virtual void get ( Log_Entry &e ) const
        {
            Sequence::get( e );

            e.add( ":t", _track );
        }

    void
    set ( Log_Entry &e )
        {
            Sequence::set( e );

            for ( int i = 0; i < e.size(); ++i )
            {
                const char *s, *v;

                e.get( i, &s, &v );

                if ( ! strcmp( ":t", s ) )
                {
                    int i;
                    sscanf( v, "%X", &i );
                    Track *t = (Track*)Loggable::find( i );

                    assert( t );

                    t->track( this );
                }
            }
        }

    Audio_Sequence ( ) : Sequence( 0, 0, 0, 0 )
        {
        }



public:

    LOG_CREATE_FUNC( Audio_Sequence );

    Audio_Sequence ( Track *track ) : Sequence( 0, 0, 0, 0, track )
        {

            _track = track;

            if ( track )
                track->add( this );

            log_create();

                                                                 /* FIXME: temporary  */
            labeltype( FL_NO_LABEL );

        }

    ~Audio_Sequence ( )
        {
            log_destroy();
        }


    Sequence * clone_empty ( void )
        {
            Audio_Sequence *t = new Audio_Sequence( track() );

            return t;
        }

//    const char *class_name ( void ) { return "Audio_Sequence"; }

    void draw ( void );
    int handle ( int m );
    void dump ( void );
    void remove_selected ( void );

    const Region *capture ( void ) const { return track()->capture(); }

    nframes_t play ( sample_t *buf, nframes_t frame, nframes_t nframes, int channels );

};
