
/*******************************************************************************/
/* Copyright (C) 2013 Mark McCurry                                             */
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

#include <FL/Fl_Box.H>

class SpectrumView : public Fl_Box
{
    static unsigned int _sample_rate;
    static float _fmin;
    static float _fmax;
    static unsigned int _nframes;

    float * _data;
    unsigned int _data_frames;
    float * _bands;
    float _dbmin;
    float _dbmax;
    bool _auto_level;

    void draw_curve ( void );
    void draw_semilog ( void );
    void analyze_data ( unsigned int plan_size );
    void clear_bands ( void );
    static void clear_plans ( void );

public:
    
    static void sample_rate ( unsigned int sample_rate );
    /* all subsequent calls to data() MUST contain this number of
     * frames. Changing this value will invalidate all cached plans */
    static void impulse_frames ( unsigned int nframes );

    /* set dB range. If min == max, then auto leveling will be enabled */
    void db_range ( float min, float max )
        {
            _dbmin = min;
            _dbmax = max;
            _auto_level = min == max;
        }

    
    /** /data/ must point to allocated memory. It will be freed when new data is set or when the control is destroyed */

    void data ( float *data, unsigned int data_frames );

    SpectrumView ( int X, int Y, int W, int H, const char *L=0 );
    virtual ~SpectrumView ( );

    virtual void resize ( int X, int Y, int W, int H );
    virtual void draw ( void );
};

