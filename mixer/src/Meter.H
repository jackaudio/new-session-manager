
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
#include <FL/Fl_Valuator.H>
#include "FL/test_press.H"

class Meter : public Fl_Valuator
{

    float _peak;
    float _value;

protected:

    virtual void draw ( void ) = 0;
    virtual int handle ( int m )
        {
            if ( m == FL_ENTER || m == FL_LEAVE )
                return 1;
            else if ( m == FL_PUSH && test_press( FL_BUTTON1 ))
            {
                reset();
                return 1;
            }

            return Fl_Valuator::handle( m );
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

public:

    Meter ( int X, int Y, int W, int H,  const char *L = 0 ) :
        Fl_Valuator( X, Y, W, H, L )
        {
            _peak = _value = -80.0f;
        }

    virtual ~Meter ( ) { }

    virtual void value ( float v )
        {
            if ( visible_r() )
                damage( FL_DAMAGE_USER1 );

            _value = v;

            if ( _value > _peak )
                _peak = _value;
        }

    virtual float value ( void ) const { return _value; }
    float peak ( void ) const { return _peak; }

    void reset ( void ) { _peak = -80.0f; redraw(); }

};
