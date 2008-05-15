
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

/* actual implementation of our side of the LASH protocol */

#include "LASH.H"

#include "debug.h"


LASH::LASH ( )
{
}

LASH::~LASH ( )
{
}


bool
LASH::handle_save_file ( const char *path )
{
    MESSAGE( "LASH wants us to save \"%s\"", path );

    return true;
}

bool
LASH::handle_restore_file ( const char *path )
{
    MESSAGE( "LASH wants us to load \"%s\"", path );

    return true;
}

void
LASH::handle_quit ( void )
{
    MESSAGE( "LASH wants us to quit" );
}
