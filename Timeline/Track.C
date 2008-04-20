
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

    if ( _name )
        free( _name );

    _name = strdup( name_field->value() );

    log_end();
}

void
Track::cb_button ( Fl_Widget *w )
{

    printf( "FIXME: inform mixer here\n" );
    if ( w == record_button )
    {
        /* FIXME: wrong place for this! */
        if ( record_button->value() )
            record_ds->start( transport.frame );
        else
            record_ds->stop( transport.frame );
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

    labeltype( FL_NO_LABEL );

    Fl_Group::size( timeline->w(), height() );

    Track *o = this;
    o->box( FL_THIN_UP_BOX );
    {
        Fl_Group *o = new Fl_Group( 2, 2, 149, 70 );
        o->color( ( Fl_Color ) 53 );
        {
            Fl_Input *o = name_field = new Fl_Input( 2, 2, 144, 24 );
            o->color( ( Fl_Color ) 33 );
            o->labeltype( FL_NO_LABEL );
            o->labelcolor( FL_GRAY0 );
            o->textcolor( 32 );

            o->callback( cb_input_field, (void*)this );

            o->hide();
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
            Fl_Box *o = new Fl_Box( 0, 76, 149, 38 );
            o->box( FL_FLAT_BOX );
            Fl_Group::current()->resizable( o );
        }

        o->size( Track::width(), h() );
        o->end();
    }
    {
        Fl_Pack *o = pack = new Fl_Pack( width(), 0, 1006, 115 );
        o->labeltype( FL_NO_LABEL );
        o->resize( x() + width(), y(), w() - width(), h() );
        Fl_Group::current()->resizable( o );

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

    /* FIXME: should be configurable, but where? */
    create_outputs( 2 );
    create_inputs( 2 );

    playback_ds = new Playback_DS( this, engine->frame_rate(), engine->nframes(), output.size() );
    record_ds = new Record_DS( this, engine->frame_rate(), engine->nframes(), input.size() );
}


Track::Track ( const char *L ) :
    Fl_Group ( 0, 0, 0, 0, L )
{

    init();

    if ( L )
        name( L );

    log_create();
}

Track::~Track ( )
{
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

    parent()->redraw();
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
Track::track ( Sequence * t )
{
    t->track( this );

    if ( track() )
        add( track() );

    _track = t;
    pack->insert( *t, 0 );

    resize();
}

void
Track::add ( Control_Sequence *t )
{
    t->track( this );

    control->add( t );

    resize();
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

    if ( ! name_field->visible() )
    {
        fl_color( FL_WHITE );
        fl_font( FL_HELVETICA, 14 );
        fl_draw( name_field->value(), name_field->x(), name_field->y(), name_field->w(), name_field->h(), FL_ALIGN_CENTER );
    }
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
        default:
            return Fl_Group::handle( m );

    }
}


/**********/
/* Engine */
/**********/

bool
Track::create_outputs ( int n )
{
    char pname[256];

    for ( int i = 0; i < n; ++i )
    {
        snprintf( pname, sizeof( pname ), "%s/out-%d", name(), i + 1 );
        output.push_back( Port( strdup( pname ), Port::Output ) );
    }

    /* FIXME: bogus */
    return true;
}

bool
Track::create_inputs ( int n )
{
    char pname[256];

    for ( int i = 0; i < n; ++i )
    {
        snprintf( pname, sizeof( pname ), "%s/in-%d", name(), i + 1 );
        input.push_back( Port( strdup( pname ), Port::Input ) );
    }

    /* FIXME: bogus */
    return true;
}




/* THREAD: RT */
nframes_t
Track::process ( nframes_t nframes )
{
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

    snprintf( pat, sizeof( pat ), "%s-%llu.wav", name(), uuid() );

    /* FIXME: hack */
    Audio_File *af = Audio_File_SF::create( pat, 48000, input.size(), "Wav/24" );

    _capture = new Region( af, track(), frame );

    /* FIXME: wrong place for this */
    _capture->_r->end = 0;
}

/* THREAD: IO */
/** write a block to the (already opened) capture file */
void
Track::write ( sample_t *buf, nframes_t nframes )
{
    _capture->write( buf, nframes );
}

#include <stdio.h>

/* THREAD: IO */
void
Track::stop ( nframes_t nframes )
{
    _capture->finalize();

    _capture = NULL;
}
