
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

/* generic master class for interfacing with LASH... */

#pragma once

namespace LASH
{
    class Client
    {
        /* to avoid including the lash header here... */
        void *_void;

    protected:

        virtual bool handle_save_file ( const char *path ) = 0;
        virtual bool handle_restore_file ( const char *path ) = 0;
        virtual void handle_quit ( void ) = 0;

    public:

        Client ( );
        virtual ~Client ( );

        bool init ( const char *jack_name, const char *full_name, int *argc, char ***argv );
        bool enabled ( void );
        void poll ( void );

        void project_save ( void );
        void project_quit ( void );

        /* TODO: project_add, project_remove, project_dir, project_name, percentage */

    };
}
