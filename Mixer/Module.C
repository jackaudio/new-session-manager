
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

#include "Module.H"
#include <FL/fl_draw.H>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Module_Parameter_Editor.H"
#include "Chain.H"



Module::Module ( int W, int H, const char *L ) : Fl_Group( 0, 0, W, H, L )
{
    init();

    log_create();
}

Module::Module ( bool is_default, int W, int H, const char *L ) : Fl_Group( 0, 0, W, H, L ), Loggable( !is_default )
{
    this->is_default( is_default );

    init();

    log_create();
}

Module::Module ( ) : Fl_Group( 0, 0, 0, 50, "Unnamed" )
{
    init();

    log_create();
}

Module::~Module ( )
{
    for ( unsigned int i = 0; i < audio_input.size(); ++i )
        audio_input[i].disconnect();
    for ( unsigned int i = 0; i < audio_output.size(); ++i )
        audio_output[i].disconnect();
    for ( unsigned int i = 0; i < control_input.size(); ++i )
        control_input[i].disconnect();
    for ( unsigned int i = 0; i < control_output.size(); ++i )
        control_output[i].disconnect();

    audio_input.clear();
    audio_output.clear();
    control_input.clear();
    control_output.clear();
}



void
Module::init ( void )
{
    _is_default = false;
    _editor = 0;
    _chain = 0;
    _instances = 1;
    box( FL_UP_BOX );
    labeltype( FL_NO_LABEL );
    clip_children( 1 );
}


void
Module::get ( Log_Entry &e ) const
{
//    e.add( ":name",            label()           );
//    e.add( ":color",           (unsigned long)color());
    {
        char *s = get_parameters();
        if ( strlen( s ) )
            e.add( ":parameter_values", s );
        delete[] s;
    }
    e.add( ":is_default", is_default() );
    e.add( ":chain", chain() );
    e.add( ":active", active() );
}

void
Module::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":chain" ) )
        {
            /* This trickiness is because we may need to know the name of
             our chain before we actually get added to it. */
            int i;
            sscanf( v, "%X", &i );
            Chain *t = (Chain*)Loggable::find( i );

            assert( t );

            chain( t );
        }
    }

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

/*         if ( ! strcmp( s, ":name" ) ) */
/*             label( v ); */
        if ( ! strcmp( s, ":parameter_values" ) )
        {
            set_parameters( v );
        }
        else if ( ! ( strcmp( s, ":is_default" ) ) )
        {
            is_default( atoi( v ) );
        }
        else if ( ! ( strcmp( s, ":active" ) ) )
        {
            if ( atoi( v ) )
                activate();
            else
                deactivate();
        }
        else if ( ! strcmp( s, ":chain" ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Chain *t = (Chain*)Loggable::find( i );

            assert( t );

            t->add( this );
        }
    }
}




/* return a string serializing this module's parameter settings.  The
   format is 1.0:2.0:... Where 1.0 is the value of the first control
   input, 2.0 is the value of the second control input etc.
*/
char *
Module::get_parameters ( void ) const
{
    char *s = new char[1024];
    s[0] = 0;
    char *sp = s;

    if ( control_input.size() )
    {
        for ( unsigned int i = 0; i < control_input.size(); ++i )
            sp += snprintf( sp, 1024 - (sp - s),"%f:", control_input[i].control_value() );

        *(sp - 1) = '\0';
    }

    return s;
}

void
Module::set_parameters ( const char *parameters )
{
    char *s = strdup( parameters );
    char *sp = s;

    char *start = s;
    int i = 0;
    for ( char *sp = s; ; ++sp )
    {
        if ( ':' == *sp || '\0' == *sp )
        {
            char was = *sp;

            *sp = '\0';

            DMESSAGE( start );

            if ( i < control_input.size() )
                control_input[i].control_value( atof( start ) );
            else
            {
                WARNING( "Module has no parameter at index %i", i );
                break;
            }

            i++;

            if ( '\0' == was  )
                break;

            start = sp + 1;
        }
    }

    free( s );
}



void
Module::draw_box ( void )
{
    fl_color( FL_WHITE );

    int tw, th, tx, ty;

    tw = w();
    th = h();
    ty = y();
    tx = x();

//    bbox( tx, ty, tw, th );

    fl_push_clip( tx, ty, tw, th );


    Fl_Color c = is_default() ? FL_BLACK : color();


    int spacing = w() / instances();
    for ( int i = instances(); i--; )
    {
        fl_draw_box( box(), tx + (spacing * i), ty, tw / instances(), th, Fl::belowmouse() == this ? fl_lighter( c ) : c );
    }

    if ( audio_input.size() && audio_output.size() )
    {
        /* maybe draw control indicators */
        if ( control_input.size() )
            fl_draw_box( FL_ROUNDED_BOX, tx + 4, ty + 4, 5, 5, is_being_controlled() ? FL_YELLOW : fl_inactive( FL_YELLOW ) );
        if ( control_output.size() )
            fl_draw_box( FL_ROUNDED_BOX, tx + tw - 8, ty + 4, 5, 5, is_controlling() ? FL_YELLOW : fl_inactive( FL_YELLOW ) );
    }

    fl_pop_clip();
//    box( FL_NO_BOX );

    Fl_Group::draw_children();
}

void
Module::draw_label ( void )
{
    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );

    const char *lp = label();

    int l = strlen( label() );

    fl_color( FL_FOREGROUND_COLOR );
    char *s = NULL;

    if ( l > 10 )
    {
        s = new char[l];
        char *sp = s;

        for ( ; *lp; ++lp )
            switch ( *lp )
            {
                case 'i': case 'e': case 'o': case 'u': case 'a':
                    break;
                default:
                    *(sp++) = *lp;
            }
        *sp = '\0';

    }

    if ( l > 20 )
        fl_font( FL_HELVETICA, 10 );
    else
        fl_font( FL_HELVETICA, 14 );

    fl_draw( s ? s : lp, tx, ty, tw, th, (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP ) );

    if ( s )
        delete[] s;
}

#include "FL/test_press.H"

int
Module::handle ( int m )
{
    switch ( m )
    {
        case FL_PUSH:
        {
            if ( test_press( FL_BUTTON1 ) )
            {
                if ( _editor )
                {
                    _editor->show();
                }
                else if ( ncontrol_inputs() )
                {

                    DMESSAGE( "Opening module parameters for \"%s\"", label() );
                    _editor = new Module_Parameter_Editor( this );

                    _editor->show();

                    do { Fl::wait(); }
                    while ( _editor->shown() );

                    DMESSAGE( "Module parameters for \"%s\" closed",label() );

                    delete _editor;

                    _editor = NULL;
                }

                return 1;
            }
            break;
        }
    }

    return Fl_Group::handle( m );
}
