
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

#pragma once

#include "Sequence.H"
#include "Control_Point.H"

class Control_Sequence : public Sequence
{

protected:


    void get ( Log_Entry &e );
    void set ( Log_Entry &e );

    Control_Sequence (  ) : Sequence( 0, 0, 1, 1 )
        {

        }

public:

    LOG_CREATE_FUNC( Control_Sequence );

    Control_Sequence ( Track *track ) : Sequence( 0, 0, 0, 0, track )
        {
            color( fl_darker( FL_GREEN ) );

            log_create();
        }

    ~Control_Sequence ( )
        {
            log_destroy();
        }

    const char *class_name ( void ) { return "Control_Sequence"; }

    void draw ( void );
    int handle ( int m );

};
