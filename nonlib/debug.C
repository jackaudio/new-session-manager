
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

#include "debug.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern char * program_invocation_short_name;

void
warnf ( warning_t level,
	   const char *module,
	   const char *file,
	   const char *function, int line, const char *fmt, ... )
{
	va_list args;
	static const char *level_tab[] = {
		"message", "\033[1;32m",
		"warning", "\033[1;33m",
		"assertion", "\033[1;31m"
	};

        module = program_invocation_short_name;

	if ( module )
		fprintf( stderr, "[\033[1;30m%s\033[0m] ", module );
#ifndef NDEBUG
	if ( file )
		fprintf( stderr, "%s", file );
	if ( line )
		fprintf( stderr, ":%i", line );
	if ( function )
		fprintf( stderr, " %s()", function );

	fprintf( stderr, ": " );
#endif

	if ( unsigned( ( level << 1 ) + 1 ) <
		 ( sizeof( level_tab ) / sizeof( level_tab[0] ) ) )
		fprintf( stderr, "%s", level_tab[( level << 1 ) + 1] );

	if ( fmt )
	{
		va_start( args, fmt );
		vfprintf( stderr, fmt, args );
		va_end( args );
	}

	fprintf( stderr, "\033[0m\n" );
}
