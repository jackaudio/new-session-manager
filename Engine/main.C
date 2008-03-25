
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


#include "const.h"
#include "Audio_File.H"
#include "Peaks.H"
#include "Loggable.H"



#include "Timeline_Server.H"

/* TODO:

   Run server process here.

   * Manage session file.
   * Keep state for all relevant objects (must mirror that in Timeline)
   * Listen for incoming connections.
   * Send and recieve 'do' messages to client(s), recording do and undo portions to journal. Serve peaks (maybe in a separate process?)

      Peak request looks like:

     >  read_peaks "foo.wav" fpp start end

      Response looks like (in binary floats):

     >  length min max min max min max


  State must be communicated to realtime thread locklessly so that:

  Realtime:

   * Manage JACK transport.
   * Manage JACK outputs.
   * Stream regions from disk.
   * Manage JACK inputs.
   * Stream tracks to disk when recording.
   * Stream control points as OSC messages. (RT safe?)
*/

int
main ( int argc, char **argv )
{
    Timeline_Server tls( 6110 );

    tls.run();
}
