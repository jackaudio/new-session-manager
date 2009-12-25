
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

class Fl_Scalepack;

class Meter_Module : public Module
{
    Fl_Scalepack *dpm_pack;

public:

    Meter_Module ( int W, int H, const char *L=0 );
    virtual ~Meter_Module ( );

    const char *name ( void ) const { return "Meter"; }

    int can_support_inputs ( int n ) { return n > 0 ? n : -1; }
    bool configure_inputs ( int n );

    static void update_cb ( void *v );
    void update_cb ( void );

protected:


    //   virtual void draw ( void );
    virtual int handle ( int m );
    virtual void process ( void );


};
