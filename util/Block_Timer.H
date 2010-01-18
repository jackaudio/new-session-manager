
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

class Block_Timer
{

    unsigned long long ts;
    const char *prefix;

    unsigned long long tv_to_ts ( timeval *tv )
        {
            return tv->tv_sec * 1e6 + tv->tv_usec;
        }

public:

    Block_Timer ( const char *prefix )
        {
            this->prefix = prefix;

            timeval tv;

            gettimeofday( &tv, NULL );

            ts = tv_to_ts( &tv );
        }

    ~Block_Timer ( )
        {
            timeval tv;

            gettimeofday( &tv, NULL );

            fprintf( stderr, "[%Lfms] %s\n", ((long double)tv_to_ts( &tv ) - ts ) / 1000, prefix );
        }
};
