
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

const char template_dir[] = "share/non-daw/templates";
const char user_template_dir[] = "~/.non-daw/templates";

#include "types.h"

class Project
{

    static int _lockfd;
    static bool _is_open;
    static char _name[256];
    static char _path[512];
    static char _created_on[40];

    static bool write_info ( void );
    static bool read_info ( int *version, nframes_t *sample_rate, char **creation_date, char **created_by );
    static void set_name ( const char *name );
    static const char *_errstr[];

public:

    enum
    {
        E_INVALID = -1,
        E_LOCKED = -2,
        E_PERM = -3,
        E_SAMPLERATE = -4,
        E_VERSION = -5
    };

    static const char *errstr ( int n ) { return _errstr[ ( 0 - n ) - 1 ]; }

    static const char *name ( void ) { return Project::_name; }
    static void compact ( void );
    static bool close ( void );
    static bool save ( void );
    static bool validate ( const char *name );
    static int open ( const char *name );
    static bool open ( void ) { return _is_open; }
    static bool create ( const char *name, const char *template_name );

    static const char *created_on ( void ) { return _created_on; }
};
