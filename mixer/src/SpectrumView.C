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
#include <string>

#include <assert.h>

static std::map<std::string,float*> _cached_plan;

float SpectrumView::_fmin = 0;
float SpectrumView::_fmax = 0;
unsigned int SpectrumView::_sample_rate = 0;

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
SpectrumView::clear_plans ( void )
{
    /* invalidate all plans */

    for ( std::map<std::string,float*>::iterator i = _cached_plan.begin();
          i != _cached_plan.end();
          i++ )
    {
        delete[] i->second;
    }

    _cached_plan.clear();
}

void
SpectrumView::sample_rate ( unsigned int sample_rate )
{
    if ( _sample_rate != sample_rate )
    {
        _sample_rate = sample_rate;
        _fmin = 10;
        _fmax = 20000;
        if ( _fmax > _sample_rate * 0.5f )
            _fmax = _sample_rate * 0.5f;

        clear_plans();
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

const char *
plan_key ( unsigned int plan_size, unsigned int nframes )
{
    static char s[256];
    snprintf( s, sizeof(s), "%d:%d", plan_size, nframes );
    return s;
}

/** Input should be an impulse response of an EQ. Output will be a
 * buffer of /bands/ floats of dB values for each frequency band */
void
SpectrumView::analyze_data ( unsigned int _plan_size )
{
    if ( ! _data )
        return;

    float res[_plan_size * 2];
    memset(res,0,sizeof(float) * _plan_size * 2);

    const char *key = plan_key( _plan_size, _nframes );

    if ( _cached_plan.find( key ) == _cached_plan.end() )
        _cached_plan[ key ] = qft_plan( _nframes, _plan_size, _sample_rate, _fmin, _fmax);

    const float *plan = _cached_plan[ key ];

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
    _nframes = 0;
    _auto_level = 0;
    _data = 0;
    _bands = 0;
    _dbmin = -70;
    _dbmax = 30;
    box(FL_FLAT_BOX); 
    color(fl_rgb_color(20,20,20));
    selection_color( fl_rgb_color( 210, 80, 80 ) );
//    end();
}

static int padding_right = 0;
static int padding_bottom = 7;

void 
SpectrumView::draw_semilog ( void )
{
    int W = w() - padding_right;
    int H = h() - padding_bottom;
    char label[50];

    fl_line_style(FL_SOLID,0);
    fl_font( FL_HELVETICA_ITALIC, 7 );

    //Db grid is easy, it is just a linear spacing
    for(int i=0; i<8; ++i) {
        int level = y()+H*i/8.0;
        fl_line(x(), level, x()+W, level);

        float value = (1-i/8.0)*(_dbmax-_dbmin) + _dbmin;
        sprintf(label, "%.1f dB", value);
        fl_draw(label, x(), level + 3, w(), 7, FL_ALIGN_RIGHT );
    }

    //The frequency grid is defined with points at
    //10,11,12,...,18,19,20,30,40,50,60,70,80,90,100,200,400,...
    //Thus we find each scale that we cover and draw the nine lines unique to
    //that scale
    float lb = 1.0f / logf( 10 );
    const int min_base = logf(_fmin)*lb;
    const int max_base = logf(_fmax)*lb;
    const float b = logf(_fmin)*lb;
    const float a = logf(_fmax)*lb-b;
    for(int i=min_base; i<=max_base; ++i) {
        for(int j=1; j<10; ++j) {
            const float freq = pow(10.0, i)*j;
            const float xloc = (logf(freq)*lb-b)/a;
            if(xloc<1.0 && xloc > -0.001)
            {
                fl_line(xloc*W+x(), y(), xloc*W+x(), y()+H);
            
                if ( j == 1 || j == 2 || j == 5 )
                {
                    sprintf(label, "%0.f %s", freq < 1000.0 ? freq : freq / 1000.0, freq < 1000.0 ? "Hz" : "KHz" );
                    int sx = x() + xloc*W + 1;
                    if ( sx < x() * W - 20 )
                        fl_draw(label, sx, y()+h());
                }
            }
        }
    }
}

void
SpectrumView::draw_curve ( void )
{
    if ( !_bands )
        return;

    int W = w() - padding_right;

    //Build lines
    float inc = 1.0f / (float)W;

    float fx = 0;
    for(  int i = 0; i < W; i++, fx += inc )
        fl_vertex(fx, 1.0f - _bands[i]);
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

