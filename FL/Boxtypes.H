
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

#include <FL/Fl.H>

#define FL_BURNISHED_OVAL_BOX    FL_FREE_BOXTYPE

#define FL_CRYSTAL_UP_BOX         (Fl_Boxtype)(FL_FREE_BOXTYPE+1)
#define FL_CRYSTAL_DOWN_BOX       (Fl_Boxtype)(FL_FREE_BOXTYPE+2)
#define FL_CRYSTAL_UP_FRAME       (Fl_Boxtype)(FL_FREE_BOXTYPE+3)
#define FL_CRYSTAL_DOWN_FRAME     (Fl_Boxtype)(FL_FREE_BOXTYPE+4)
#define FL_CRYSTAL_THIN_UP_BOX    (Fl_Boxtype)(FL_FREE_BOXTYPE+5)
#define FL_CRYSTAL_THIN_DOWN_BOX  (Fl_Boxtype)(FL_FREE_BOXTYPE+6)
#define FL_CRYSTAL_ROUND_UP_BOX   (Fl_Boxtype)(FL_FREE_BOXTYPE+7)
#define FL_CRYSTAL_ROUND_DOWN_BOX (Fl_Boxtype)(FL_FREE_BOXTYPE+8)

void init_boxtypes ( void );
