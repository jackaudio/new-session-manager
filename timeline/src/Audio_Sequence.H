
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


#pragma once

#include "Sequence.H"
#include "Audio_Region.H"

#include <FL/Fl_Input.H>
class Audio_Sequence_Header;

class Audio_Sequence : public Sequence
{

protected:

    void get ( Log_Entry &e ) const;

    void set ( Log_Entry &e );

    Audio_Sequence ( ) : Sequence( 0 )
        {
            init();
        }

    void handle_widget_change ( nframes_t start, nframes_t length );

    void draw ( void );
    int handle ( int m );

    static void cb_button ( Fl_Widget *w, void *v );
    void cb_button ( Fl_Widget *w );

    void init ( void );

public:

    LOG_CREATE_FUNC( Audio_Sequence );

    Audio_Sequence_Header * header ( void ) { return (Audio_Sequence_Header*)child(0); }

    Audio_Sequence ( Track *track, const char *name = 0 );
    ~Audio_Sequence ( );

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_DEFAULT; }

    virtual Sequence * clone_empty ( void )
	{
            Audio_Sequence *t = new Audio_Sequence( track() );

            return t;
	}
	
    virtual Sequence * clone_empty ( const char *name )
        {
            Audio_Sequence *t = new Audio_Sequence( track(), name );

            return t;
        }

    virtual void name ( const char *s );
    virtual const char *name ( void ) const;

    const Audio_Region *capture_region ( void ) const;

    nframes_t play ( sample_t *buf, nframes_t frame, nframes_t nframes, int channels );

};
