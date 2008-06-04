
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

#include "Log_Entry.H"

Log_Entry::Log_Entry ( )
{
    _sa = (char**)malloc( sizeof( char * ) );
    *_sa = NULL;
    _i = 0;
}

Log_Entry::Log_Entry ( char **sa )
{
    _sa = sa;
    _i = 0;

    if ( _sa )
        while ( _sa[ _i ] ) ++_i;

}

Log_Entry::~Log_Entry ( )
{
    if ( ! _sa )
        return;

    for ( _i = 0; _sa[ _i ]; ++_i )
    {
        free( _sa[ _i ] );
    }

    free( _sa );
}

void
Log_Entry::grow (  )
{
    _sa = (char**)realloc( _sa, sizeof( char * ) * (_i + 2) );
    _sa[ _i + 1 ] = NULL;
}
\
int
Log_Entry::size ( void ) const
{
    return _i;
}

void
Log_Entry::get ( int n, const char **name, const char **value )
{
    *name = _sa[ n ];
    *value = *name + strlen( *name ) + 1;
}


char **
Log_Entry::sa ( void )
{
    char **sa = _sa;

    _sa = NULL;

    return sa;
}
