

/*******************************************************************************/
/* Copyright (C) 2007,2008 Jonathan Moore Liles                                */
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

// #pragma once

typedef unsigned char byte_t;
typedef double tick_t;
typedef unsigned int uint;


/* #define min(x,y) ((x) < (y) ? (x) : (y)) */
/* #define max(x,y) ((x) > (y) ? (x) : (y)) */

#include <algorithm>
using namespace std;

#define elementsof(x) (sizeof((x)) / sizeof((x)[0]))

#include "config.h"
#include "const.h"
#include "debug.h"
