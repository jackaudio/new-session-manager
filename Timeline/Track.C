
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

#include "Track.H"

#include "Transport.H"
#include "Playback_DS.H"
#include "Record_DS.H"

#include "Engine.H"

#include "Port.H"

#include "../FL/Fl_Sometimes_Input.H"
#include <FL/fl_ask.H>
#include <FL/Fl_Color_Chooser.H>
// #include <FL/fl_draw.H>
#include <FL/Fl.H>

int Track::_soloing = 0;

const char *Track::capture_format = "Wav 24";

void
Track::cb_input_field ( Fl_Widget *w, void *v )
{
    ((Track*)v)->cb_input_field();
}

void
Track::cb_button ( Fl_Widget *w, void *v )
{
    ((Track*)v)->cb_button( w );
}


void
Track::cb_input_field ( void )
{
    log_start();

    name( name_field->value() );

    log_end();
}

void
Track::cb_button ( Fl_Widget *w )
{

    if ( w == record_button )
    {

    }
    if ( w == mute_button )
    {

    }
    if ( w == solo_button )
    {
        if ( solo_button->value() )
            ++_soloing;
        else
            --_soloing;
    }
    else
        if ( w == take_menu )
        {
            int v = take_menu->value();

            switch ( v )
            {
                case 0:                                         /* show all takes */
                    show_all_takes( take_menu->menu()[ v ].value() );
                    return;
                case 1:                                         /* new */
                    track( track()->clone_empty() );
                    return;
            }

            const char *s = take_menu->menu()[ v ].text;

            for ( int i = takes->children(); i--; )
            {
                Sequence *t = (Sequence*)takes->child( i );
                if ( ! strcmp( s, t->name() ) )
                {
                    track( t );
                    redraw();
                    break;
                }
            }
        }
}

void
Track::init ( void )
{
    _capture = NULL;
    _track = NULL;
    _name = NULL;
    _selected = false;
    _show_all_takes = false;
    _size = 1;

    record_ds = NULL;
    playback_ds = NULL;

    labeltype( FL_NO_LABEL );

    Fl_Group::size( timeline->w(), height() );

    Track *o = this;
    o->box( FL_THIN_UP_BOX );
    {
        Fl_Group *o = new Fl_Group( 0, 0, 149, 70 );
        o->color( ( Fl_Color ) 53 );

        {
            Fl_Input *o = name_field = new Fl_Sometimes_Input( 2, 2, 144, 24 );
            o->color( FL_BACKGROUND_COLOR );
            o->labeltype( FL_NO_LABEL );
            o->labelcolor( FL_GRAY0 );
            o->textcolor( FL_FOREGROUND_COLOR );

            o->callback( cb_input_field, (void*)this );
        }

        {
            Fl_Group *o = controls = new Fl_Group( 2, 28, 149, 24 );

            {
                Fl_Button *o = record_button =
                    new Fl_Button( 6, 28, 26, 24, "@circle" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->selection_color( FL_RED );
                o->labelsize( 8 );
                o->callback( cb_button, this );
            }
            {
                Fl_Button *o = mute_button =
                    new Fl_Button( 35, 28, 26, 24, "m" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
                o->callback( cb_button, this );
            }
            {
                Fl_Button *o = solo_button =
                    new Fl_Button( 66, 28, 26, 24, "s" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
                o->callback( cb_button, this );
            }
            {
                Fl_Menu_Button *o = take_menu =
                    new Fl_Menu_Button( 97, 28, 47, 24, "T" );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
                o->callback( cb_button, this );

                o->add( "Show all takes", 0, 0, 0, FL_MENU_TOGGLE );
                o->add( "New", 0, 0, 0, FL_MENU_DIVIDER );
            }
            o->end();
        }

        {
            Fl_Box *o = new Fl_Box( 0, 72, 149, 38 );
            o->box( FL_NO_BOX );
            Fl_Group::current()->resizable( o );
        }

        o->size( Track::width(), h() );
        o->end();
    }
    {
        Fl_Pack *o = pack = new Fl_Pack( width(), 0, 1006, 115 );
        o->labeltype( FL_NO_LABEL );
        o->resize( x() + width(), y(), w() - width(), h() );

        resizable( o );

        {
            Fl_Pack *o = control = new Fl_Pack( width(), 0, pack->w(), 115 );
            o->end();
        }

        {
            Fl_Pack *o = takes = new Fl_Pack( width(), 0, pack->w(), 115 );
            o->end();
            o->hide();
        }

        o->end();
    }
    end();

}


Track::Track ( const char *L, int channels ) :
    Fl_Group ( 0, 0, 0, 0, 0 )
{
    init();

    if ( L )
        name( L );

    color( (Fl_Color)rand() );

    configure_inputs( channels );
    configure_outputs( channels );

    log_create();
}

Track::~Track ( )
{
    /* FIXME: why is this necessary? */
    timeline->remove_track( this );

    log_destroy();
}


static int pack_visible( Fl_Pack *p )
{
    int v = 0;
    for ( int i = p->children(); i--; )
        if ( p->child( i )->visible() )
            v++;

    return v;
}

/* adjust size of widget and children */
void
Track::resize ( void )
{
    for ( int i = takes->children(); i--; )
        takes->child( i )->size( w(), height()  );

    for ( int i = control->children(); i--; )
        control->child( i )->size( w(), height()  );

    if ( _show_all_takes )
    {
        takes->show();
        Fl_Group::size( w(), height() * ( 1 + takes->children() + pack_visible( control ) ) );
    }
    else
    {
        takes->hide();
        Fl_Group::size( w(), height() * ( 1 + pack_visible( control ) ) );
    }

    if ( track() )
        track()->size( w(), height() );


    if ( controls->y() + controls->h() > y() + h() )
        controls->hide();
    else
        controls->show();

    /* FIXME: why is this necessary? */
    if ( parent() )
        parent()->parent()->redraw();
}

void
Track::size ( int v )
{
    if ( v < 0 || v > 3 )
        return;

    _size = v;

    resize();
}


void
Track::add ( Sequence * t )
{
    takes->insert( *t, 0 );
    if ( ! t->name() )
    {
        char pat[20];
        snprintf( pat, sizeof( pat ), "%d", takes->children() );
        t->name( strdup( pat ) );
    }

    take_menu->add( t->name() );

    t->labeltype( FL_ENGRAVED_LABEL );
}

void
Track::remove ( Sequence *t )
{
    takes->remove( t );

    resize();

//            take_menu->remove( t->name() );
}

void
Track::remove ( Control_Sequence *t )
{
    control->remove( t );

    resize();
}

void
Track::track ( Sequence * t )
{
    t->track( this );

    if ( track() )
        add( track() );

    _track = t;
    pack->insert( *t, 0 );

    t->labeltype( FL_NO_LABEL );

    resize();
}

void
Track::add ( Control_Sequence *t )
{
    printf( "adding control sequence\n" );

    t->track( this );

    control->add( t );

    control_out.push_back( new Port( Port::Output, name(), control_out.size(), "cv" ) );

    t->output( control_out.back() );

    resize();
}

/** add all widget on this track falling within the given rectangle to
    the selection.  */
void
Track::select ( int X, int Y, int W, int H,
                bool include_control, bool merge_control )
{

    Sequence *t = track();

    if ( ! ( t->y() > Y + H || t->y() + t->h() < Y ) )
        t->select_range( X, W );
    else
        include_control = true;

    if ( include_control )
        for ( int i = control->children(); i--; )
        {
            Control_Sequence *c = (Control_Sequence*)control->child( i );

            if ( merge_control ||
                 ( c->y() >= Y && c->y() + c->h() <= Y + H  ) )
                c->select_range( X, W );
        }
}

void
Track::draw ( void )
{
    if ( _selected )
    {
        Fl_Color c = color();

        color( FL_RED );

        Fl_Group::draw();

        color( c );
    }
    else
        Fl_Group::draw();
}

int
Track::handle ( int m )
{
    Logger log( this );

    switch ( m )
    {
        case FL_MOUSEWHEEL:
        {
            if ( ! Fl::event_shift() )
                return 0;

            int d = Fl::event_dy();

            printf( "%d\n", d );

            if ( d < 0 )
                size( size() - 1 );
            else
                size( size() + 1 );

            return 1;
        }
        case FL_PUSH:
        {

            int X = Fl::event_x();
            int Y = Fl::event_y();

            if ( Fl::event_button3() && X < Track::width() )
            {
                int c = output.size();

                /* context menu */
                Fl_Menu_Item menu[] =
                    {
                        { "Type",            0, 0, 0, FL_SUBMENU    },
                        { "Mono",            0, 0, 0, FL_MENU_RADIO | ( c == 1 ? FL_MENU_VALUE : 0 ) },
                        { "Stereo",          0, 0, 0, FL_MENU_RADIO | ( c == 2 ? FL_MENU_VALUE : 0 ) },
                        { "Quad",            0, 0, 0, FL_MENU_RADIO | ( c == 4 ? FL_MENU_VALUE : 0 ) },
                        { "...",             0, 0, 0, FL_MENU_RADIO | ( c == 3 || c > 4 ? FL_MENU_VALUE : 0 ) },
                        { 0                  },
                        { "Add Control"      },
                        { "Color"            },
                        { "Remove",          0, 0, 0, transport->rolling ? FL_MENU_INACTIVE : 0 },
                        { 0 },
                    };

                const Fl_Menu_Item *r = menu->popup( X, Y, "Track" );

                if ( r && r > &menu[ 0 ] )
                {
                    if ( r < &menu[ 4 ] )
                    {
                        int c = r - &menu[1];
                        int ca[] = { 1, 2, 4 };

                        configure_inputs( ca[ c ] );
                        configure_outputs( ca[ c ] );
                    }
                    else
                    {
                        if ( r == &menu[ 4 ] )
                        {
                            const char *s = fl_input( "How many channels?", "3" );

                            int c = atoi( s );

                            if ( c <= 0 || c > 10 )
                                fl_alert( "Invalid number of channels." );
                            else
                            {
                                configure_inputs( c );
                                configure_outputs( c );
                            }
                        }
                        else if ( r == &menu[ 6 ] )
                        {
                            new Control_Sequence( this );
                        }
                        else if ( r == &menu[ 7 ] )
                        {
                            unsigned char r, g, b;

                            Fl::get_color( color(), r, g, b );

                            if ( fl_color_chooser( "Track Color", r, g, b ) )
                            {
                                color( fl_rgb_color( r, g, b ) );
                            }

//                            color( fl_show_colormap( color() ) );
                            redraw();
                        }
                        else if ( r == &menu[ 8 ] )
                        {
                            int r = fl_choice( "Are you certain you want to remove track \"%s\"?", "Cancel", NULL, "Remove", name() );

                            if ( r == 2 )
                            {
                                timeline->remove_track( this );
                                /* FIXME: need to queue deletion. in a way that won't crash the playback. */
//                                delete this;
                                Fl::delete_widget( this );
                            }
                        }
                    }
                }

                return 1;
            }
        }
        default:
            return Fl_Group::handle( m );

    }

    return 0;
}


/**********/
/* Engine */
/**********/

void
Track::update_port_names ( void )
{
    for ( int i = 0; i < output.size(); ++i )
        output[ i ].name( name(), i );

    for ( int i = 0; i < input.size(); ++i )
        input[ i ].name( name(), i );

    for ( int i = 0; i < control_out.size(); ++i )
        control_out[ i ]->name( name(), i, "cv" );


/*     /\* tell any attached control sequences to do the same *\/ */
/*     for ( int i = control->children(); i-- ) */
/*         ((Control_Sequence*)control->child( i ))->update_port_names(); */

}

bool
Track::configure_outputs ( int n )
{
    int on = output.size();

    if ( n == on )
        return true;

//    engine->lock();

    if ( playback_ds )
    {
        Playback_DS *ds = playback_ds;
        playback_ds = NULL;

        ds->shutdown();
        delete ds;
    }

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            Port p( Port::Output, name(), i );

            if ( p.valid() )
                output.push_back( p );
            else
                printf( "error: could not create output port!\n" );
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            output.back().shutdown();
            output.pop_back();
        }
    }


    playback_ds = new Playback_DS( this, engine->frame_rate(), engine->nframes(), output.size() );

//    engine->unlock();
    /* FIXME: bogus */
    return true;
}

bool
Track::configure_inputs ( int n )
{
    int on = input.size();

    if ( n == on )
        return true;

//    engine->lock();

    if ( record_ds )
    {
        Record_DS *ds = record_ds;
        record_ds = NULL;

        ds->shutdown();
        delete ds;
    }

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            Port p( Port::Input, name(), i );

            if ( p.valid() )
                input.push_back( p );
            else
                printf( "error: could not create input port!\n" );
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            input.back().shutdown();
            input.pop_back();
        }
    }

    record_ds = new Record_DS( this, engine->frame_rate(), engine->nframes(), input.size() );

//    engine->unlock();

    /* FIXME: bogus */
    return true;
}

/* THREAD: RT */
nframes_t
Track::process ( nframes_t nframes )
{

    if ( ! transport->rolling )
    {
        for ( int i = output.size(); i--; )
            output[ i ].silence( nframes );

        for ( int i = input.size(); i--; )
            input[ i ].silence( nframes );

        /* FIXME: is this really the right thing to do for control ports? */
        for ( int i = control_out.size(); i--; )
            control_out[ i ]->silence( nframes );

        return 0;
    }

    for ( int i = control->children(); i--; )
        ((Control_Sequence*)control->child( i ))->process( nframes );

    if ( playback_ds )
    {
        record_ds->process( nframes );
        return playback_ds->process( nframes );
    }
    else
        return 0;
}

/* THREAD: RT */
void
Track::seek ( nframes_t frame )
{
    if ( playback_ds )
        return playback_ds->seek( frame );
}



/* FIXME: what about theading issues with this region/audiofile being
   accessible from the UI thread? Need locking? */

#include "Region.H"


#include <time.h>

/** very cheap UUID generator... */
unsigned long long
uuid ( void )
{
    time_t t = time( NULL );

    return (unsigned long long) t;
}


/* THREAD: IO */
/** create capture region and prepare to record */
void
Track::record ( nframes_t frame )
{
    assert( _capture == NULL );

    char pat[256];

    snprintf( pat, sizeof( pat ), "%s-%llu", name(), uuid() );

    _capture_af = Audio_File_SF::create( pat, engine->sample_rate(), input.size(), Track::capture_format );

    if ( ! _capture_af )
    {
        /* ERROR */

    }

    /* open it again for reading in the GUI thread */
    Audio_File *af = Audio_File::from_file( _capture_af->name() );

    _capture = new Region( af, track(), frame );

    _capture->prepare();
}

/* THREAD: IO */
/** write a block to the (already opened) capture file */
void
Track::write ( sample_t *buf, nframes_t nframes )
{
    nframes_t l = _capture_af->write( buf, nframes );

    _capture->write( l );
}

#include <stdio.h>

/* THREAD: IO */
void
Track::stop ( nframes_t frame )
{
    _capture->finalize( frame );

    _capture = NULL;
}
