
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

#include <string.h>
#include <stdio.h>

void unescape_url ( char *url )
{
    char *r, *w;

    r = w = url;

    for ( ; *r; r++, w++ )
    {
        if ( *r == '%' )
        {
            char data[3] = { *(r + 1), *(r + 2), 0 };

            int c;

            sscanf( data, "%2X", &c );

            *w = c;

            r += 2;
        }
        else
            *w = *r;
    }

    *w = 0;
}

char *escape_url ( const char *url )
{
    const char *s;
    char *w;

    char r[1024];

    s = url;

    w = r;

    for ( ; *s && w < r + sizeof( r ); s++, w++ )
    {
        switch ( *s )
        {
            case ' ':
            case '<':
            case '>':
            case '%':
            case '#':
            case '*':
            case ',':
                sprintf( w, "%%%2X", *s );
                w += 2;
                break;
            default:
                *w = *s;
                break;
                
        }
    }

    *w = 0;
    
    return strdup( r );
}
