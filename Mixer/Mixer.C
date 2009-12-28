
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

/* This is the main mixer group. It contains and manages Mixer_Strips. */

#include "Mixer.H"
#include "Mixer_Strip.H"

#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Bar.H>

#include "Engine/Engine.H"

#include "Project.H"

#include <string.h>
#include "debug.h"

const double STATUS_UPDATE_FREQ = 0.2f;

static Fl_Pack *mixer_strips;


#include "util/debug.h"

static void update_cb( void *v ) {
    Fl::repeat_timeout( STATUS_UPDATE_FREQ, update_cb, v );

    ((Mixer*)v)->update();
}

Mixer::Mixer ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    box( FL_NO_BOX );
    labelsize( 96 );
    { Fl_Menu_Bar *o = new Fl_Menu_Bar( X, Y, W, 24 );
        o->add( "&Project/&New" );
        o->add( "&Project/&Open" );
        o->add( "&Project/&Quit" );
        o->add( "&Mixer/&Add Strip" );
        o->add( "&Options" );
    }
    { Fl_Scroll *o = scroll = new Fl_Scroll( X, Y + 24, W, H - 24 );
        o->box( FL_NO_BOX );
        o->type( Fl_Scroll::HORIZONTAL_ALWAYS );
        {
            Fl_Pack *o = mixer_strips = new Fl_Pack( X, Y + 24, W, H - 18 - 24 );
            label( "Non-Mixer" );
            align( (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE) );
            o->box( FL_NO_BOX );
            o->type( Fl_Pack::HORIZONTAL );
            o->spacing( 2 );
            o->end();
            Fl_Group::current()->resizable( o );
        }
        o->end();
        Fl_Group::current()->resizable( o );
    }

    end();

//    Fl::add_timeout( STATUS_UPDATE_FREQ, update_cb, this );

    MESSAGE( "Scanning for plugins..." );

}

Mixer::~Mixer ( )
{
    /* FIXME: teardown */

}

void Mixer::resize ( int X, int Y, int W, int H )
{
    Fl_Group::resize( X, Y, W, H );

    mixer_strips->resize( X, Y + 24, W, H - 18 - 24 );

    scroll->resize( X, Y + 24, W, H - 24 );
}

void Mixer::add ( Mixer_Strip *ms )
{
    MESSAGE( "Add mixer strip \"%s\"", ms->name() );

    engine->lock();

    mixer_strips->add( ms );
//    mixer_strips->insert( *ms, 0 );

    engine->unlock();

    scroll->redraw();

//    redraw();
}

void Mixer::remove ( Mixer_Strip *ms )
{
    MESSAGE( "Remove mixer strip \"%s\"", ms->name() );

    engine->lock();

    mixer_strips->remove( ms );

    engine->unlock();

    delete ms;
    parent()->redraw();
}

bool
Mixer::contains ( Mixer_Strip *ms )
{
    return ms->parent() == mixer_strips;
}

void Mixer::update ( void )
{
    THREAD_ASSERT( UI );

    for ( int i = mixer_strips->children(); i--; )
    {
        ((Mixer_Strip*)mixer_strips->child( i ))->update();
    }
    // redraw();
}

void
Mixer::process ( unsigned int nframes )
{
    THREAD_ASSERT( RT );

    for ( int i = mixer_strips->children(); i--; )
    {
        ((Mixer_Strip*)mixer_strips->child( i ))->process( nframes );
    }
}

/** retrun a pointer to the track named /name/, or NULL if no track is named /name/ */
Mixer_Strip *
Mixer::track_by_name ( const char *name )
{
    for ( int i = mixer_strips->children(); i-- ; )
    {
        Mixer_Strip *t = (Mixer_Strip*)mixer_strips->child( i );

        if ( ! strcmp( name, t->name() ) )
            return t;
    }

    return NULL;
}

/** return a malloc'd string representing a unique name for a new track */
char *
Mixer::get_unique_track_name ( const char *name )
{
    char pat[256];

    strcpy( pat, name );

    for ( int i = 1; track_by_name( pat ); ++i )
        snprintf( pat, sizeof( pat ), "%s.%d", name, i );

    return strdup( pat );
}


void
Mixer::snapshot ( void )
{
    for ( int i = 0; i < mixer_strips->children(); ++i )
        ((Mixer_Strip*)mixer_strips->child( i ))->log_children();
}


void
Mixer::new_strip ( void )
{
    engine->lock();

    add( new Mixer_Strip( get_unique_track_name( "Unnamed" ), 1 ) );

    engine->unlock();

//    scroll->size( mixer_strips->w(), scroll->h() );
}

bool
Mixer::save ( void )
{
    MESSAGE( "Saving state" );
    Loggable::snapshot_callback( &Mixer::snapshot, this );
    Loggable::snapshot( "save.mix" );
    return true;
}


int
Mixer::handle ( int m )
{
    int r = Fl_Group::handle( m );

    switch ( m )
    {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        case FL_SHORTCUT:
        {
            if ( Fl::event_key() == 'a' )
            {
                new_strip();
                return 1;
            }
            else if ( Fl::event_ctrl() && Fl::event_key() == 's' )
            {
//                save();
                Project::save();
                return 1;
            }
            else
                return r;
            break;
        }
        default:
            return r;
            break;
    }

    // return 0;
    return r;
}
