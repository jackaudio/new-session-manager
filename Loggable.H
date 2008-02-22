
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

/* Master class for journaling. */

#pragma once

#include <stdio.h>

class Loggable
{

    static FILE *_fp;
    static int _log_id;

private:
    int _id;

public:

    static bool open ( const char *filename );

    Loggable ( )
        {
            _id = ++_log_id;
        }

    /* log messages for journal */
    virtual void log_create ( void ) = 0;
    virtual void log_destroy ( void ) = 0;
    virtual void log_move ( void ) = 0;
    virtual void log_change ( void ) = 0;

    void log ( const char *module, const char *action, const char *fmt, ... );

};

#ifndef _LOGGABLE_C
#define log( act, fmt, args... ) log( __CLASS__, act, fmt, ## args )
#endif
