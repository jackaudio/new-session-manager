
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

#define _LOGGABLE_C
#include "Loggable.H"
#undef _LOGABLE_C

#include <stdio.h>
#include <stdarg.h>

FILE *Loggable::_fp;

int Loggable::_log_id = 0;

bool
Loggable::open ( const char *filename )
{
    if ( ! ( Loggable::_fp = fopen( filename, "a+" ) ) )
    {
        printf( "Could not open log file for writing!" );
        return false;
    }

    return true;
}

void
Loggable::log ( const char *module, const char *action, const char *fmt, ... )
{
    va_list args;

    fprintf( _fp, "%-15s %-8s %p ", module, action, _id );

    if ( fmt )
    {
        va_start( args, fmt );
        vfprintf( _fp, fmt, args );
        va_end( args );
    }

    fprintf( _fp, "\n" );
}
