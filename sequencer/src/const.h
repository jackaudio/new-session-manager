
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

#pragma once

/* getting around this will require bank switching etc, and
   before that happens I'd like to see a song with 128 phrases in it. */
const int MAX_PHRASE = 128;
const int MAX_PATTERN = 128;

const unsigned int PPQN = 480;

/* interval between GUI updates for playhead movement, etc. */
const double TRANSPORT_POLL_INTERVAL = 0.02;

const char APP_NAME[]   = "Non-Sequencer";
const char APP_TITLE[]  = "The Non-Sequencer";
const char COPYRIGHT[]  = "Copyright (c) 2007-2013 Jonathan Moore Liles";

#define PACKAGE "non-sequencer"

/* directories */

#define USER_CONFIG_DIR ".non/"
#define INSTRUMENT_DIR "instruments/"
