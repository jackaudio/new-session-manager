
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



Time_Point::Time_Point ( ) : _time( 4, 4 )
{
    timeline->time_track->add( this );
}

Time_Point::Time_Point ( nframes_t when, int bpb, int note ) : _time( bpb, note )
{
    _make_label();

    timeline->time_track->add( this );

    start( when );

    log_create();
}

Time_Point::Time_Point ( const Time_Point &rhs ) : Sequence_Point( rhs )
{
    _time = rhs._time;

    log_create();

}

Time_Point::~Time_Point ( )
{
    timeline->time_track->remove( this );

    log_destroy();
}



void
Time_Point::get ( Log_Entry &e ) const
{
//    Sequence_Point::get( e );

    e.add( ":start", start() );
    e.add( ":beats_per_bar", _time.beats_per_bar );
    e.add( ":beat_type",     _time.beat_type     );
}

void
Time_Point::set ( Log_Entry &e )
{

    Sequence_Point::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":beats_per_bar" ) )
            _time.beats_per_bar = atoi( v );
        else if ( ! strcmp( s, ":beat_type" ) )
            _time.beat_type = atoi( v );

/*         /\* FIXME: we need to add this to the time track on creation!!! *\/ */
/*         timeline->time_track->add( this ); */

    }

    sequence()->handle_widget_change( start(), length() );

    _make_label();
}

int
Time_Point::handle ( int m )
{
    Logger log( this );

    if ( m == FL_PUSH && Fl::event_button3() && ! ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) ) )
    {

        time_sig t = _time;

        edit( &t );

        time( t.beats_per_bar, t.beat_type );

        return 0;

    }

    return Sequence_Point::handle( m );
}

#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Menu_Window.H>

class Time_Point_Editor : public Fl_Menu_Window
{

    /* not permitted */
    Time_Point_Editor ( const Time_Point_Editor &rhs );
    Time_Point_Editor & operator = ( const Time_Point_Editor &rhs );

    time_sig *_sig;
    Fl_Int_Input *_beats;
    Fl_Int_Input *_beat_type;

    bool _sucess;

public:

    Time_Point_Editor ( time_sig *sig )
        : Fl_Menu_Window( 150, 110, "Edit Time" )
        {
            _sig = sig;

            set_modal();

            {
                Fl_Int_Input *o = _beats = new Fl_Int_Input( 50, 0 + 24, 50, 24, "Beats Per Bar:" );
                o->align( FL_ALIGN_TOP );
                o->when( FL_WHEN_NOT_CHANGED | FL_WHEN_ENTER_KEY );
                o->callback( &Time_Point_Editor::enter_cb, (void*)this );
            }

            {
                Fl_Int_Input *o = _beat_type = new Fl_Int_Input( 50, 0 + 75, 50, 24, "Beat Type:" );
                o->align( FL_ALIGN_TOP );
                o->when( FL_WHEN_NOT_CHANGED | FL_WHEN_ENTER_KEY );
                o->callback( &Time_Point_Editor::enter_cb, (void*)this );
            }

            char pat[10];
            snprintf( pat, sizeof( pat ), "%d", _sig->beats_per_bar );

            _beats->value( pat );

            snprintf( pat, sizeof( pat ), "%d", _sig->beat_type );

            _beat_type->value( pat );

            end();

            show();

            while ( shown() )
                Fl::wait();
        }

    static void
    enter_cb ( Fl_Widget *, void *v )
        {
            ((Time_Point_Editor*)v)->enter_cb();
        }

    void
    enter_cb ( void )
        {
            _sig->beats_per_bar = atoi( _beats->value() );
            _sig->beat_type = atoi( _beat_type->value() );

            _sucess = true;

            hide();
        }

    bool
    sucess ( void )
        {
            return _sucess;
        }
};

bool
Time_Point::edit ( time_sig *sig )
{
    Time_Point_Editor ti( sig );

    return ti.sucess();
}
