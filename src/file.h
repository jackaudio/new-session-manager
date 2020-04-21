
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

#include <stdio.h>

unsigned long modification_time ( const char *file );

bool newer ( const char *file1, const char *file2 );
unsigned long size ( const char *file );
int exists ( const char *name );
bool acquire_lock ( int *lockfd, const char *filename );
void release_lock ( int *lockfd, const char *filename );
int backwards_fgetc ( FILE *fp );
char * backwards_afgets ( FILE *fp );
void touch ( int fd );
void write_line ( const char *dir, const char *name, const char *value );
char * read_line ( const char *dir, const char *name );
size_t free_space ( const char *file );
size_t total_space ( const char *file );
int percent_used ( const char *file );
