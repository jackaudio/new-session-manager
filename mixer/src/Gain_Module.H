
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

#include "Module.H"
#include "dsp.h"

class Gain_Module : public Module
{
    Value_Smoothing_Filter smoothing;

public:

    Gain_Module ( );
    Gain_Module ( const Gain_Module & rhs );
    virtual ~Gain_Module ( );

    const char *name ( void ) const { return "Gain"; }

    int can_support_inputs ( int n ) { return n; }
    bool configure_inputs ( int n );

    LOG_CREATE_FUNC( Gain_Module );

    MODULE_CLONE_FUNC( Gain_Module );

    virtual void handle_sample_rate_change ( nframes_t n );

protected:

    virtual void process ( nframes_t nframes );

};
