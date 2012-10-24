
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

#include "Sequence_Point.H"


class Cursor_Point : public Sequence_Point
{
    char *_type;

protected:

//    const char *class_name ( void ) { return "Time_Point"; }

    virtual void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );
    void log_children ( void ) const;

    Cursor_Point ( );

public:

    LOG_CREATE_FUNC( Cursor_Point );
    SEQUENCE_WIDGET_CLONE_FUNC( Cursor_Point );

//    static bool edit ( time_sig *sig );

    Cursor_Point ( nframes_t when, const char *type, const char *label );
    Cursor_Point ( const Cursor_Point &rhs );

    ~Cursor_Point ( );

    const char * type ( void ) const { return _type; }
    void type ( const char *v )
        { 
            if ( _type ) 
                free( _type );
            
            _type = NULL;
            
            if ( v )
                _type = strdup( v );
        }
                
    int handle ( int m );

};
