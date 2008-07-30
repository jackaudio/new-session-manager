
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

#include <stdio.h>

unsigned long modification_time ( const char *file );

bool newer ( const char *file1, const char *file2 );
unsigned long size ( const char *file );
int exists ( const char *name );
bool acquire_lock ( int *lockfd, const char *filename );
void release_lock ( int *lockfd, const char *filename );
int backwards_fgetc ( FILE *fp );
char * backwards_fgets ( char *s, int size, FILE *fp );
void touch ( int fd );
void write_line ( const char *dir, const char *name, const char *value );
void read_line ( const char *dir, const char *name, char **value );
size_t free_space ( const char *file );
size_t total_space ( const char *file );
int percent_used ( const char *file );
