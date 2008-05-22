
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

#include "Timeline.H"
#include "Sequence_Region.H"

class Audio_File;
class Peaks_Redraw_Request;

class Audio_Region : public Sequence_Region
{

    /* not permitted */
    Audio_Region & operator = ( const Audio_Region &rhs );

public:

    static bool inherit_track_color;

    struct Fade
    {
        enum fade_type_e { Linear = 0, Sigmoid, Logarithmic, Parabolic };
        enum fade_dir_e { In, Out };

        fade_type_e type;
        nframes_t length;

        Fade ( )
            {
                type   = Linear;
                length = 0;
            }

        bool
        operator< ( const Fade &rhs ) const
            {
                return length < rhs.length;
            }

        float increment ( void ) const
            {
                return 1.0f / (float)length;
            }

        /** Return gain for frame /index/ of /nframes/ on a gain curve
         * of type /type/.*/
        /* FIXME: calling a function per sample is bad, switching on
         * type mid fade is bad. */
        inline float
        gain ( const float fi ) const
            {
                switch ( type )
                {
                    case Linear:
                        return fi;
                    case Sigmoid:
                        return (1.0f - cos( fi * M_PI )) / 2.0f;
                    case Logarithmic:
                        return pow( 0.1f, (1.0f - fi) * 3.0f );
                    case Parabolic:
                        return 1.0f - (1.0f - fi) * (1.0f - fi);
                    default:
                        return 1.0f;
                }
            }

        void apply ( sample_t *buf, fade_dir_e dir, long start, nframes_t end, nframes_t nframes ) const;
    };

/*     struct Fade_In : public Fade; */
/*     struct Fade_Out : public Fade; */

private:

    Audio_File *_clip;                                          /* clip this region represents */

    float _scale;                                               /* amplitude adjustment */

    Fade _fade_in;
    Fade _fade_out;

    friend class Track;                                  /* for _clip */


    static void peaks_pending_cb ( void *v );
    void peaks_pending_cb ( Peaks_Redraw_Request *r );

protected:

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

public:

    static Fl_Boxtype _box;
    static Fl_Color _selection_color;
    Fl_Color selection_color ( void ) const { return _selection_color; }
    void selection_color ( Fl_Color v ) { _selection_color = v; }

    void init ( void );

    Audio_Region ( )
        {
            init();
        }

    bool current ( void ) const { return this == belowmouse(); }

public:

    const char * source_name ( void ) const;

    LOG_CREATE_FUNC( Audio_Region );

    SEQUENCE_WIDGET_CLONE_FUNC( Audio_Region );

    ~Audio_Region ( )
        {
            log_destroy();
        }

    Fl_Boxtype box ( void ) const { return Audio_Region::_box; }
    Fl_Align align ( void ) const { return  (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_BOTTOM /*| FL_ALIGN_CLIP*/ | FL_ALIGN_INSIDE); }

    Audio_Region ( const Audio_Region & rhs );
    Audio_Region ( Audio_File *c );
    Audio_Region ( Audio_File *c, Sequence *t, nframes_t o );

    void draw_fade ( const Fade &fade, Fade::fade_dir_e dir, bool filled, int X, int W );


    int handle ( int m );
    void draw_box( void );
    void draw ( void );
    void resize ( void );

    void normalize ( void );


    /* Engine */
    nframes_t read ( sample_t *buf, nframes_t pos, nframes_t nframes, int channel ) const;
    nframes_t write ( nframes_t nframes );
    void prepare ( void );
    bool finalize ( nframes_t frame );

};
