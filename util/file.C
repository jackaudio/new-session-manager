
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

unsigned long
mtime ( const char *file )
{
    struct stat st;

    if ( stat( file, &st ) )
        return 0;

    return st.st_mtime;
}

/** returns /true/ if /file1/ is newer than /file2/ (or file2 doesn't exist) */
bool
newer ( const char *file1, const char *file2 )
{
    return mtime( file1 ) > mtime( file2 );
}

unsigned long
size ( const char *file )
{
    struct stat st;

    if ( stat( file, &st ) )
        return 0;

    return st.st_size;
}

int
exists ( const char *name )
{
    struct stat st;

    return 0 == stat( name, &st );
}

bool
acquire_lock ( int *lockfd, const char *filename )
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    *lockfd = ::creat( filename, 0777 );

    if ( fcntl( *lockfd, F_SETLK, &fl ) != 0 )
        return false;

    return true;
}

void
release_lock ( int *lockfd, const char *filename )
{
    unlink( filename );

    ::close( *lockfd );

    *lockfd = 0;
}
