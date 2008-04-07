
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

#ifndef Track_Header_H
#define Track_Header_H
#include <FL/Fl.H>
#include "Track.H"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Box.H>

#include "Loggable.H"

#include "Port.H"
#include <vector>
using std::vector;

class Track_Header : public Fl_Group, public Loggable
{

public:

    Track_Header ( int X, int Y, int W, int H, const char *L = 0 );
    ~Track_Header ( );

private:

//    Track * _track;

    char *_name;

    bool _selected;

    bool _show_all_takes;

    int _size;

    enum { AUDIO } _type;

    Track *_track;

public:

    Fl_Input * name_field;
    Fl_Button *record_button;
    Fl_Button *mute_button;
    Fl_Button *solo_button;
    Fl_Menu_Button *take_menu;
    Fl_Group *controls;

    Fl_Pack *pack;
    Fl_Pack *control;
    Fl_Pack *takes;


    vector <Port> output;                                       /* output ports... */


    const char *class_name ( void ) { return "Track_Header"; }

    void set ( char **sa )
        {
            for ( int i = 0; sa[i]; ++i )
            {
                char *s = sa[i];

                strtok( s, " " );

                char *v = s + strlen( s ) + 1;

                if  ( *v == '"' )
                {
                    v++;
                    v[ strlen( v ) - 2 ] = '\0';
                }

                if ( ! strcmp( s, ":h" ) )
                {
                    size( atoi( v ) );

                    Fl_Widget::size( w(), height() );
                }
                else
                    if ( ! strcmp( s, ":selected" ) )
                        _selected = atoi( v );
                    else
                        if ( ! strcmp( s, ":name" ) )
                        {
                            _name = strdup( v );
                            name_field->value( _name );
                        }
                        else
                            if ( ! strcmp( s, ":track" ) )
                            {
                                int i;
                                sscanf( v, "%X", &i );
                                Track *t = (Track*)Loggable::find( i );

                                assert( t );

                                track( t );
                            }


                free( s );
            }

            free( sa );

        }

    char ** get ( void )
        {
            char **sa = (char**)malloc( sizeof( char* ) * (1 + 4) );

            int i = 0;

            asprintf( &sa[ i++ ], ":name \"%s\"", _name ? _name : ""  );
            asprintf( &sa[ i++ ], ":track 0x%X", track() ? track()->id() : 0 );
            asprintf( &sa[ i++ ], ":selected %d", _selected );
/*             asprintf( &sa[ i++ ], ":record %d", record_button->value() ); */
/*             asprintf( &sa[ i++ ], ":solo %d", solo_button->value() ); */
/*             asprintf( &sa[ i++ ], ":mute %d", mute_button->value() ); */
            asprintf( &sa[ i++ ], ":h %d", size() );
//            asprintf( &sa[ i++ ], ":gain %f", _scale );

            sa[ i ] = NULL;

            return sa;
        }




    /* for loggable */
    static Loggable *
    create ( char **sa )
        {
            Track_Header *r = new Track_Header( 0, 0, 1, 1 );

            r->set( sa );

            return (Loggable *)r;
        }

    void
    draw ( void )
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

    void add_control( Track *t );

    int size ( void ) const { return _size; }

    void resize ( void );
    void size ( int v );

    int height ( void ) const
        {
            static int table[] = { 30, 80, 150, 300 };

            return table[ _size ];
        }

    void show_all_takes ( bool b )
        {
            _show_all_takes = b;
            resize();
        }

    const char * name ( void ) const { return _name; }
    bool mute ( void ) const { return mute_button->value(); }
    bool solo ( void ) const { return solo_button->value(); }
    bool arm ( void ) const { return record_button->value(); }
    bool selected ( void ) const { return _selected; }

    static void cb_input_field ( Fl_Widget *w, void *v );
    void cb_input_field ( void );
    static void cb_button ( Fl_Widget *w, void *v );
    void cb_button ( Fl_Widget *w );


    static int width ( void ) { return 150; }

    void track( Track * t );
    Track * track ( void ) { return _track; }

    void add ( Track * t )
        {
            takes->insert( *t, 0 );
            if ( ! t->name() )
            {
                char pat[20];
                snprintf( pat, sizeof( pat ), "%d", takes->children() );
                t->name( strdup( pat ) );
                take_menu->add( t->name() );
            }

        }

    void remote ( Track *t )
        {
            takes->remove( t );
//            take_menu->remove( t->name() );
        }

    int handle ( int m )
        {
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

};
#endif
