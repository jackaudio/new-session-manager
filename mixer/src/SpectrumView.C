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

#include "SpectrumView.H"
#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <math.h>

#include <cstdlib>
#include <cstring>
#include <map>

#include <assert.h>

static std::map<int,float*> _cached_plan;

float SpectrumView::_fmin = 10;
float SpectrumView::_fmax = 24000;
unsigned int SpectrumView::_sample_rate = 48000;


void
SpectrumView::clear_bands ( void )
{
    if ( _bands )
        delete[] _bands;
    
    _bands = NULL;
}

void
SpectrumView::data ( float *data, unsigned int nframes )
{
    if ( _data )
        delete[] _data;

    _data = data;
    _nframes = nframes;

    clear_bands();

    redraw();
}

void
SpectrumView::sample_rate ( unsigned int sample_rate )
{
    if ( _sample_rate != sample_rate )
    {
        _sample_rate = sample_rate;
        _fmin = 10;
        _fmax = _sample_rate * 0.5f;

        /* invalidate all plans */

        for ( std::map<int,float*>::iterator i = _cached_plan.begin();
              i != _cached_plan.end();
              i++ )
        {
            delete[] i->second;
        }

        _cached_plan.clear();
    }
}


#define min(a,b) (a<b?a:b)
#define max(a,b) (a<b?b:a)


static float*
qft_plan ( unsigned frames, unsigned samples, float Fs, float Fmin, float Fmax )
{
    float *op = new float[ frames * samples * 2 ];
    
    //Our scaling function must be some f(0) = Fmin and f(1) = Fmax
    // Thus,
    // f(x)=10^(a*x+b)  -> b=log(Fmin)/log(10)
    // log10(Fmax)=a+b  -> a=log(Fmax)/log(10)-b
    
    const float b = logf(Fmin)/logf(10);
    const float a = logf(Fmax)/logf(10)-b;

    //Evaluate at set frequencies
    const float one_over_samples = 1.0f / samples;
    const float one_over_samplerate = 1.0f / Fs;
  
    for(unsigned i=0; i<samples; ++i) 
    {
        const float F = powf(10.0,a*i*one_over_samples+b)*one_over_samplerate;
        const float Fp = -2*M_PI*F;
        
        float Fpj = 0;

        for(unsigned j = 0; j < frames; ++j, Fpj += Fp )
        {
            const unsigned ji = i*frames*2+2*j;
            
            op[ji+0] = sinf(Fpj);
            op[ji+1] = cosf(Fpj);
        }
    }

    return op;
}

/** Input should be an impulse response of an EQ. Output will be a
 * buffer of /bands/ floats of dB values for each frequency band */
void
SpectrumView::analyze_data ( unsigned int _plan_size )
{
    float res[_plan_size * 2];
    memset(res,0,sizeof(float) * _plan_size * 2);

    if ( _cached_plan.find( _plan_size ) == _cached_plan.end() )
        _cached_plan[_plan_size ] = qft_plan( _nframes, _plan_size, _sample_rate, _fmin, _fmax);

    const float *plan = _cached_plan[_plan_size];

    //Evaluate at set frequencies
    for(unsigned i=0; i<_plan_size; ++i) {
        unsigned ti = i*2;
        unsigned tif = ti*_nframes;
        for(unsigned int j=0; j < _nframes ; ++j) {
            unsigned ji = tif+j*2;

            res[ti+0] += plan[ji+0]*_data[j];
            res[ti+1] += plan[ji+1]*_data[j];
        }
    }

  
    float *result = new float[_plan_size];
    for(unsigned i=0; i<_plan_size; ++i) {
        const float abs_ = sqrtf(res[2*i]*res[2*i]+res[2*i+1]*res[2*i+1]);
        result[i] = 20*logf(abs_)/logf(10);
    }
    
    {
        if ( _auto_level )
        {
            /* find range and normalize */
            float _min=1000, _max=-1000;
            for(unsigned int i=0; i< _plan_size; ++i)
            {
                _min = min(_min, result[i]);
                _max = max(_max, result[i]);
            }
        
            _dbmin = _min;
            _dbmax = _max;
        }
    
        double minS = 1.0 / (_dbmax-_dbmin);

        for( unsigned int i=0; i<_plan_size; ++i)
            result[i] = (result[i]-_dbmin)*minS;
    }

    clear_bands();
        
    _bands = result;
}

SpectrumView::~SpectrumView ( void )
{
    clear_bands();
    if ( _data )
        delete[] _data;
}

SpectrumView::SpectrumView ( int X, int Y, int W, int H, const char *L )
    : Fl_Box(X,Y,W,H,L)
{
    _auto_level = 0;
    _data = 0;
    _nframes = 0;
    _bands = 0;
    _dbmin = -70;
    _dbmax = 30;
    box(FL_FLAT_BOX); 
    color(fl_rgb_color(20,20,20));
    selection_color( fl_rgb_color( 210, 80, 80 ) );
//    end();
}

static int padding_right = 0;
static int padding_bottom = 0;

void 
SpectrumView::draw_semilog ( void )
{
    int W = w() - padding_right;
    int H = h() - padding_bottom;

    /* char dash[] = {5,5 }; */
    /* fl_line_style(0, 1, dash); */
    fl_line_style(FL_SOLID,0);

    //Db grid is easy, it is just a linear spacing
    for(int i=0; i<8; ++i) {
        int level = y()+H*i/8.0;
        fl_line(x(), level, x()+W, level);
    }

    //The frequency grid is defined with points at
    //10,11,12,...,18,19,20,30,40,50,60,70,80,90,100,200,400,...
    //Thus we find each scale that we cover and draw the nine lines unique to
    //that scale
    const int min_base = logf(_fmin)/logf(10);
    const int max_base = logf(_fmax)/logf(10);
    const float b = logf(_fmin)/logf(10);
    const float a = logf(_fmax)/logf(10)-b;
    for(int i=min_base; i<=max_base; ++i) {
        for(int j=1; j<10; ++j) {
            const float freq = pow(10.0, i)*j;
            const float xloc = (logf(freq)/logf(10)-b)/a;
            if(xloc<1.0 && xloc > -0.001)
                fl_line(xloc*W+x(), y(), xloc*W+x(), y()+H);
        }
    }

    fl_end_line();
            
    fl_font( FL_HELVETICA_ITALIC, 7 );
    //Place the text labels
    char label[256];
    for(int i=0; i<8; ++i) {
        int level = (y()+H*i/8.0) + 3;
        float value = (1-i/8.0)*(_dbmax-_dbmin) + _dbmin;
        sprintf(label, "%.1f dB", value);
//        fl_draw(label, x()+w() + 3, level);
        fl_draw(label, x(), level, w(), 7, FL_ALIGN_RIGHT );
    }
            
    for(int i=min_base; i<=max_base; ++i) {
        {
            const float freq = pow(10.0, i)*1;
            const float xloc = (logf(freq)/logf(10)-b)/a;
            sprintf(label, "%0.f %s", freq < 1000.0 ? freq : freq / 1000.0, freq < 1000.0 ? "Hz" : "KHz" );
            if(xloc<1.0)
                fl_draw(label, xloc*W+x()+1, y()+h());
        }
    }
}

void
SpectrumView::draw_curve ( void )
{
    int W = w() - padding_right;

    //Build lines
    float inc = 1.0 / (float)W;

    float fx = 0;
    for(  int i = 0; i < W; i++, fx += inc )
        fl_vertex(fx, 1.0 - _bands[i]);
}

void
SpectrumView::draw ( void ) 
{
    //Clear Widget
    Fl_Box::draw();

    int W = w() - padding_right;
    int H = h() - padding_bottom;
    
    if ( !_bands ) {
        analyze_data( W );
    }

    //Draw grid
    fl_color(fl_color_add_alpha(fl_rgb_color( 100,100,100), 50 ));

    draw_semilog();

    fl_push_clip( x(),y(),W,H);

            
    fl_color(fl_color_add_alpha( selection_color(), 20 ));
   
    fl_push_matrix();
    fl_translate( x(), y() + 2 );
    fl_scale( W,H- 2 );

    fl_begin_polygon();
    
    fl_vertex(0.0,1.0);

    draw_curve();

    fl_vertex(1.0,1.0);
                  
    fl_end_polygon();

    fl_color(fl_color_add_alpha( selection_color(), 100 ));
    fl_begin_line();
    fl_line_style(FL_SOLID,2);
    
    /* fl_vertex(0.0,1.0); */

    draw_curve();

    /* fl_vertex(1.0,1.0); */

    fl_end_line();
    
    fl_pop_matrix();

    fl_line_style(FL_SOLID,0);

    fl_pop_clip();
}

void
SpectrumView::resize ( int X, int Y, int W, int H )
{
    if ( W != w() )
        clear_bands();

    Fl_Box::resize(X,Y,W,H);
}

