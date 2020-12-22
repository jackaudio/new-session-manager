
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")     */
/* Copyright (C) 2020- Nils Hilbricht                                          */
/*                                                                             */
/* This file is part of New-Session-Manager                                    */
/*                                                                             */
/* New-Session-Manager is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation, either version 3 of the License, or           */
/* (at your option) any later version.                                         */
/*                                                                             */
/* New-Session-Manager is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/* GNU General Public License for more details.                                */
/*                                                                             */
/* You should have received a copy of the GNU General Public License           */
/* along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.*/
/*******************************************************************************/

/* debug.h
 *
 * 11/21/2003 - Jonathan Moore Liles
 *
 * Debuging support.
 *
 * Disable by defining the preprocessor variable NDEBUG prior to inclusion.
 *
 * The following macros sould be defined as string literals
 *
 *  name            value
 *
 *  __MODULE__      Name of module. eg. "libfoo"
 *
 *  __FILE__        Name of file. eg. "foo.c"
 *
 *  __FUNCTION__        Name of enclosing function. eg. "bar"
 *
 *  (inteter literal)
 *  __LINE__        Number of enclosing line.
 *
 *
 * __FILE__, and __LINE__ are automatically defined by standard CPP
 * implementations. __FUNCTION__ is more or less unique to GNU, and isn't
 * strictly a preprocessor macro, but rather a reserved word in the compiler.
 * There is a sed script available with this toolset that is able to fake
 * __FUNCTION__ (among other things) with an extra preprocesessing step.
 *
 * __MODULE__ is nonstandard and should be defined the enclosing program(s).
 * Autoconf defines PACKAGE as the module name, and these routines will use its
 * value instead if __MODULE__ is undefined.
 *
 * The following routines are provided (as macros) and take the same arguments
 * as printf():
 *
 * MESSAGE( const char *format, ... )
 * WARNING( const char *format, ... )
 * FATAL( const char *format, ... )
 *
 * Calling MESSAGE or WARNING prints the message to stderr along with module,
 * file and line information, as well as appropriate emphasis. Calling
 * FATAL will do the same, and then call abort() to end the program. It is
 * unwise to supply any of these marcros with arguments that produce side
 * effects. As, doing so will most likely result in Heisenbugs; program
 * behavior that changes when debugging is disabled.
 *
 */




#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef __MODULE__
#ifdef PACKAGE
#define __MODULE__ PACKAGE
#else
#define __MODULE__ NULL
#endif
#endif

#ifndef __GNUC__
    #define __FUNCTION__ NULL
#endif

extern bool quietMessages;

typedef enum {
    W_MESSAGE = 0,
    W_WARNING,
    W_FATAL
} warning_t;

void
warnf ( warning_t level,
       const char *module,
       const char *file,
        const char *function, int line, const char *fmt, ... );

//We do not use NDEBUG anymore. Messages are a command line switch.
//Warnings, asserts and errors are always important.

// #define MESSAGE( fmt, args... ) warnf( W_MESSAGE, __MODULE__, __FILE__, __FUNCTION__, __LINE__, fmt, ## args )
#define MESSAGE( fmt, args... ) do { if ( ! (quietMessages) ) { warnf( W_MESSAGE, __MODULE__, __FILE__, __FUNCTION__, __LINE__, fmt, ## args ); } } while ( 0 )
#define WARNING( fmt, args... ) warnf( W_WARNING, __MODULE__, __FILE__, __FUNCTION__, __LINE__, fmt, ## args )
#define FATAL( fmt, args... ) ( warnf( W_FATAL, __MODULE__, __FILE__, __FUNCTION__, __LINE__, fmt, ## args ), abort() )
#define ASSERT( pred, fmt, args... ) do { if ( ! (pred) ) { warnf( W_FATAL, __MODULE__, __FILE__, __FUNCTION__, __LINE__, fmt, ## args ); abort(); } } while ( 0 )

#endif
