
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int
backwards_fgetc ( FILE *fp )
{
    int c;

    if ( fseek( fp, -1, SEEK_CUR ) != 0 )
        return -1;

    c = fgetc( fp );

    fseek( fp, -1, SEEK_CUR );

    return c;
}

char *
backwards_fgets ( char *s, int size, FILE *fp )
{
    if ( fseek( fp, -1, SEEK_CUR ) != 0 )
        return NULL;

    int c;
    while ( ( c = backwards_fgetc( fp ) ) >= 0 )
        if ( '\n' == c )
            break;

    long here = ftell( fp );

    fseek( fp, 1, SEEK_CUR );

    char *r = fgets( s, size, fp );

    fseek( fp, here, SEEK_SET );

    return r;
}


/** update the modification time of file referred to by /fd/ */
void
touch ( int fd )
{
    struct stat st;

    fstat( fd, &st );

    fchmod( fd, st.st_mode );
}

/** write a single string to a file */
void
write_line ( const char *dir, const char *name, const char *value )
{
    char path[512];

    snprintf( path, sizeof( path ), "%s/%s", dir, name );

    FILE *fp = fopen( path, "w" );

    if ( ! fp )
        return;

    fputs( value, fp );

    fclose( fp );
}

/** write a single string to a file */
void
read_line ( const char *dir, const char *name, char **value )
{
    char path[512];

    *value = 0;

    snprintf( path, sizeof( path ), "%s/%s", dir, name );

    FILE *fp = fopen( path, "r" );

    if ( ! fp )
        return;

    *value = (char*)malloc( 512 );

    fgets( *value, 512, fp );

    fclose( fp );
}
