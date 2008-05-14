
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
#include "Tempo_Map_Sequence.H"
#include "Timeline.H" // for timeline->tempo_track

void
Tempo_Point::get ( Log_Entry &e ) const
{
//    Sequence_Point::get( e );

    e.add( ":start", start() );
    e.add( ":tempo", _tempo );
}

void
Tempo_Point::set ( Log_Entry &e )
{

    Sequence_Point::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":tempo" ) )
            _tempo = atof( v );

/*         /\* FIXME: we need to add this to the time track on creation!!! *\/ */
/*         timeline->tempo_track->add( this ); */

    }

    timeline->redraw();

    _make_label();
}


Tempo_Point::Tempo_Point ( )
{
    timeline->tempo_track->add( this );
}

Tempo_Point::Tempo_Point ( nframes_t when, float bpm )
{
    _tempo = bpm;
    _r->start = when;

    _make_label();

    timeline->tempo_track->add( this );

    log_create();
}



Tempo_Point::~Tempo_Point ( )
{
    log_destroy();
}



int
Tempo_Point::handle ( int m )
{
    if ( m == FL_PUSH && Fl::event_button3() && ! ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) ) )
    {
        float t = _tempo;
        edit( &t );
        tempo( t );
        return 0;
    }

    int r = Sequence_Widget::handle( m );

    if ( m == FL_RELEASE )
    {
        sequence()->sort();
        timeline->redraw();
    }
    return r;
}


#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Menu_Window.H>


class Tempo_Point_Editor : public Fl_Menu_Window
{

    /* not permitted */
    Tempo_Point_Editor ( const Tempo_Point_Editor &rhs );
    Tempo_Point_Editor & operator = ( const Tempo_Point_Editor &rhs );

    float *_tempo;
    Fl_Float_Input *_fi;

    bool _sucess;

public:

    Tempo_Point_Editor ( float *tempo ) : Fl_Menu_Window(  75, 58, "Edit Tempo" )
        {
            _sucess = false;
            _tempo = tempo;

            set_modal();

            Fl_Float_Input *fi = _fi = new Fl_Float_Input( 12, 0 + 24, 50, 24, "Tempo:" );
            fi->align( FL_ALIGN_TOP );
            fi->when( FL_WHEN_ENTER_KEY );
            fi->callback( &Tempo_Point_Editor::enter_cb, (void*)this );

            char pat[10];
            snprintf( pat, sizeof( pat ), "%.1f", *tempo );

            fi->value( pat );

            end();

            show();

            while ( shown() )
                Fl::wait();
        }

    static void
    enter_cb ( Fl_Widget *w, void *v )
        {
            ((Tempo_Point_Editor*)v)->enter_cb();
        }

    void
    enter_cb ( void )
        {
            sscanf( _fi->value(), "%f", _tempo );
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
Tempo_Point::edit ( float *tempo )
{
    Tempo_Point_Editor ti( tempo );

    return ti.sucess();
}
