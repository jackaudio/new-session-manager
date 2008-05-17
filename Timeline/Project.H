
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

class Project
{

    static bool _is_open;
    static char _name[256];
    static char _path[512];

public:

    static bool write_info ( void );
    static bool read_info ( void );
    static const char *name ( void ) { return Project::_name; }
    static void set_name ( const char *name );
    static bool close ( void );
    static bool validate ( const char *name );
    static bool open ( const char *name );
    static bool open ( void ) { return _is_open; }
    static bool create ( const char *name, const char *template_name );

};
