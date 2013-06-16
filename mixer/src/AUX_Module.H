
/*******************************************************************************/
/* Copyright (C) 2013 Jonathan Moore Liles                                     */
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

#include "JACK_Module.H"
#include "dsp.h"

class AUX_Module : public JACK_Module
{
    int _number;

    Value_Smoothing_Filter smoothing;

protected:

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

public:

    virtual const char *name ( void ) const { return "AUX"; }

    virtual bool configure_outputs ( int n );
    virtual bool configure_inputs ( int n );

    void number ( int n );
    int number ( void ) const { return _number; }

    AUX_Module ( );
    virtual ~AUX_Module ( );

    LOG_CREATE_FUNC( AUX_Module );
    
    virtual void handle_sample_rate_change ( nframes_t n );

protected:

    virtual void draw ( void );
    virtual void process ( nframes_t nframes );

};

