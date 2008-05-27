
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

static const char *event_names[] =
{
    "FL_NO_EVENT",
    "FL_PUSH",
    "FL_RELEASE",
    "FL_ENTER",
    "FL_LEAVE",
    "FL_DRAG",
    "FL_FOCUS",
    "FL_UNFOCUS",
    "FL_KEYDOWN",
    "FL_KEYUP",
    "FL_CLOSE",
    "FL_MOVE",
    "FL_SHORTCUT",
    "FL_DEACTIVATE",
    "FL_ACTIVATE",
    "FL_HIDE",
    "FL_SHOW",
    "FL_PASTE",
    "FL_SELECTIONCLEAR",
    "FL_MOUSEWHEEL",
    "FL_DND_ENTER",
    "FL_DND_DRAG",
    "FL_DND_LEAVE",
    "FL_DND_RELEASE",
};

const char *
event_name ( int m )
{
    return event_names[ m ];
}
