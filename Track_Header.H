
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

#include "Loggable.H"

class Track_Header : public Fl_Group, public Loggable
{

public:

    Track_Header ( int X, int Y, int W, int H, const char *L = 0 );
    ~Track_Header ( );

private:

    Track * _track;

    char *_name;

    bool _selected;

public:

    Fl_Input * name_field;
    Fl_Button *record_button;
    Fl_Button *mute_button;
    Fl_Button *solo_button;
    Fl_Menu_Button *take_menu;

    Fl_Pack *takes;

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

                            _track = t;
                        }


                free( s );
            }

            free( sa );

        }

    char ** get ( void )
        {
            char **sa = (char**)malloc( sizeof( char* ) * (1 + 3) );

            int i = 0;

            asprintf( &sa[ i++ ], ":name \"%s\"", _name ? _name : ""  );
            asprintf( &sa[ i++ ], ":track 0x%X", _track ? _track->id() : 0 );
            asprintf( &sa[ i++ ], ":selected %d", _selected );
//            asprintf( &sa[ i++ ], ":gain %f", _scale );

            sa[ i ] = NULL;

            return sa;
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

    const char * name ( void ) const { return _name; }
    bool mute ( void ) const { return mute_button->value(); }
    bool solo ( void ) const { return solo_button->value(); }
    bool arm ( void ) const { return record_button->value(); }
    bool selected ( void ) const { return _selected; }

    static void cb_input_field ( Fl_Widget *w, void *v );
    void cb_input_field ( void );

    static int width();

    void track( Track * t );

};
#endif
