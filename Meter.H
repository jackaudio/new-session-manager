
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

/* Base class for all meters */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

class Meter : public Fl_Widget
{

    float _peak;
    float _old_value;
    float _value;

protected:

    virtual void draw ( void ) = 0;
    virtual int handle ( int m )
        {
            if ( m == FL_PUSH )
            {
//                if ( Fl::event_button3() )
//                    hide();
//                else
                    reset();
            }

            return 0;
        }

    float
    deflection ( float db )
        {
            float def = 0.0f;

            if ( db < -70.0f )
                def = 0.0f;
            else if ( db < -60.0f )
                def = ( db + 70.0f ) * 0.25f;
            else if ( db < -50.0f )
                def = ( db + 60.0f ) * 0.5f + 2.5f;
            else if ( db < -40.0f )
                def = ( db + 50.0f ) * 0.75f + 7.5f;
            else if ( db < -30.0f )
                def = ( db + 40.0f ) * 1.5f + 15.0f;
            else if ( db < -20.0f )
                def = ( db + 30.0f ) * 2.0f + 30.0f;
            else if ( db < 6.0f )
                def = ( db + 20.0f ) * 2.5f + 50.0f;
            else
                def = 115.0f;

            return def / 115.0f;
        }

    float old_value ( void ) const { return _old_value; }

public:

    Meter ( int X, int Y, int W, int H,  const char *L = 0 ) :
        Fl_Widget( X, Y, W, H, L )
        {
            _peak = _value = -80.0f;
            _old_value = 4.0f;
        }

    virtual ~Meter ( ) { }

    void value ( float v )
        {
            if ( _value != v )
            {
                damage( FL_DAMAGE_USER1 );

                _old_value = _value;
                _value = v;

                if ( _value > _peak )
                    _peak = _value;
            }
        }

    float value ( void ) const { return _value; }
    float peak ( void ) const { return _peak; }

    void reset ( void ) { _peak = -80.0f; redraw(); }

};

#include <FL/Fl_Group.H>
#include <stdio.h>


/* ... Extension methods for any group containing only meters. Access
 * via a cast to (Meter_Pack *) */

class Meter_Pack : public Fl_Group
{

public:

/** return a pointer to the meter for channel /c/ in group of meters /g/ */
    Meter *
    channel ( int c )
        {
            if ( c > children() )
            {
                fprintf( stderr, "no such channel\n" );
                return NULL;
            }

            return (Meter *)child( c );
        }

    int
    channels ( void ) const { return children(); }

};
