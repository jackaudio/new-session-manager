
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
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

#pragma once

#include "Sequence_Region.H"
#include "Cursor_Sequence.H"

class Cursor_Region : public Sequence_Region
{
    /* not permitted */
    Cursor_Region & operator = ( const Cursor_Region &rhs );

    char *_type;

protected:

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

    Cursor_Region ( )
        {
            _label = NULL;
            _type = NULL;
        }

    Cursor_Region ( const Cursor_Region &rhs );

public:

    virtual Fl_Color box_color ( void ) const;
    virtual void box_color ( Fl_Color c );

    /* for loggable */
    LOG_CREATE_FUNC( Cursor_Region );
    SEQUENCE_WIDGET_CLONE_FUNC( Cursor_Region );

    Cursor_Region ( nframes_t when, nframes_t length, const char *type, const char *label );
    virtual ~Cursor_Region ( );

    void draw_box ( void );
    void draw ( void );
    int handle ( int m );


    const char * type ( void ) const { return _type; }
    void type ( const char *v )
        { 
            if ( _type ) 
                free( _type );
            
            _type = NULL;
            
            if ( v )
                _type = strdup( v );
        }

};
