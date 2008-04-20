
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


#include "Tempo_Point.H"
#include "Tempo_Sequence.H"
#include "Timeline.H" // for timeline->tempo_track

void
Tempo_Point::get ( Log_Entry &e )
{
    e.add( ":x", _r->offset );
    e.add( ":tempo", _tempo );
}

void
Tempo_Point::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":x" ) )
            _r->offset = atol( v );
        else if ( ! strcmp( s, ":tempo" ) )
            _tempo = atof( v );

        /* FIXME: we need to add this to the time track on creation!!! */
        timeline->tempo_track->add( this );
    }

    timeline->redraw();

    _make_label();
}


Tempo_Point::Tempo_Point ( nframes_t when, float bpm )
{
    _tempo = bpm;
    _r->offset = when;

    _make_label();

    log_create();
}



Tempo_Point::~Tempo_Point ( )
{
    if ( _label ) delete[] _label;
    log_destroy();
}


int
Tempo_Point::handle ( int m )
{
    int r = Sequence_Widget::handle( m );

    if ( m == FL_RELEASE )
    {
        _track->sort();
        timeline->redraw();
    }
    return r;
}
