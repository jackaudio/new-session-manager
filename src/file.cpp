
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")     */
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/vfs.h>

unsigned long
modification_time ( const char *file )
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
    return modification_time( file1 ) > modification_time( file2 );
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
backwards_afgets ( FILE *fp )
{
    size_t size = 0;

    char *s = NULL;
    size_t i = 0;
    int c;
    while ( ( c = backwards_fgetc( fp ) ) >= 0 )
    {
        if ( i > 0 && '\n' == c )
            break;

        if ( i >= size )
        {
            size += 256;
            s = (char*)realloc( s, size );
        }

        s[i++] = c;

    }

    if ( s )
    {
        s[i] = 0;

        int len = strlen(s) ;
        int c, i, j;

        for (i = 0, j = len - 1; i < j; i++, j--)
        {
            c = s[i];
            s[i] = s[j];
            s[j] = c;
        }
    }

    fseek( fp, 1, SEEK_CUR );

    return s;
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
char *
read_line ( const char *dir, const char *name  )
{
    char path[512];

    snprintf( path, sizeof( path ), "%s/%s", dir, name );

    FILE *fp = fopen( path, "r" );

    if ( ! fp )
        return 0;

    char *value = (char*)malloc( 512 );

    value[0] = 0;

    fgets( value, 512, fp );

    fclose( fp );

    return value;
}

#include <sys/statvfs.h>

/** return the number of blocks free on filesystem containing file named /file/ */
fsblkcnt_t
free_space ( const char *file )
{
    struct statfs st;
    memset( &st, 0, sizeof( st ) );

    statfs( file, &st );

    return st.f_bavail;
}

/** return the total number of blocks on filesystem containing file named /file/ */
fsblkcnt_t
total_space ( const char *file )
{
    struct statfs st;
    memset( &st, 0, sizeof( st ) );

    statfs( file, &st );

    return st.f_blocks;
}

/** return the percentage of usage on filesystem containing file named /file/ */
int
percent_used ( const char *file )
{
    const double ts = total_space( file );
    const double fs = free_space( file );

    double percent_free = ( ( fs / ts ) * 100.0f );

    return (int) (100.0f - percent_free);
}
