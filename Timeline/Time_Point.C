
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

#include "Time_Point.H"
#include "Time_Sequence.H"
#include "Timeline.H" // for timeline->time_track

void
Time_Point::get ( Log_Entry &e ) const
{
    e.add( ":x",             _r->offset          );
    e.add( ":beats_per_bar", _time.beats_per_bar );
    e.add( ":beat_type",     _time.beat_type     );
}

void
Time_Point::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":x" ) )
            _r->offset = atol( v );
        else if ( ! strcmp( s, ":beats_per_bar" ) )
            _time.beats_per_bar = atoi( v );
        else if ( ! strcmp( s, ":beat_type" ) )
            _time.beat_type = atoi( v );

        /* FIXME: we need to add this to the time track on creation!!! */
        timeline->time_track->add( this );
    }

    timeline->redraw();

    _make_label();
}
