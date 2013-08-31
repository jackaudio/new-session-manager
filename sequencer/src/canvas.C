
/*******************************************************************************/
/* Copyright (C) 2007-2008 Jonathan Moore Liles                                */
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

/* This is a generic double-buffering, optimizing canvas interface to
   grids (patterns and phrases). It draws only what is necessary to keep
   the display up-to-date. Actual drawing functions are in draw.C */
#include <FL/Fl.H>
#include <FL/Fl_Cairo.H>

#include "canvas.H"
#include "pattern.H"
#include "common.h"

#include "non.H"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include "gui/ui.H"
#include <FL/Fl_Panzoomer.H>
#include <FL/Fl_Slider.H>

using namespace MIDI;
extern UI *ui;

extern Fl_Color velocity_colors[];
const int ruler_height = 14;




class Canvas::Canvas_Panzoomer : public Fl_Panzoomer
{
    Fl_Offscreen backbuffer;

public:

    Canvas_Panzoomer( int X, int Y,int W, int H, const char *L = 0 )
        : Fl_Panzoomer(X,Y,W,H,L)
        {
            backbuffer = 0;
        }

    Canvas *canvas;

private:

    static void draw_dash ( tick_t x, int y, tick_t l, int color, int selected, void *userdata )
        {
            Canvas_Panzoomer *o = (Canvas_Panzoomer*)userdata;
            
            o->draw_dash( x,y,l,color,selected );
        }

    void draw_dash ( tick_t x, int y, tick_t w, int color, int selected ) const
        {
            if ( selected )
                color = FL_MAGENTA;
            else
                color = velocity_colors[ color ];
            
            Canvas *c = canvas;
    
            double VS = (double)this->h() / c->m.maxh;
            double HS = (double)this->w() / ( _xmax - _xmin );

            y = c->ntr( y );
            
            if ( y < 0 )
                return;
            
            y *= VS;
            
            fl_color( fl_color_average( color, FL_GRAY, 0.5 ) );

            fl_rectf(
                x * HS,
                y,
                (w * HS) + 0.5,
                1 * VS + 0.5 );
        }
    
protected:

    void draw_background ( int X, int Y, int W, int H )
        {

            /* DMESSAGE( "%s%s%s%s%s%s", */
            /*           damage() & FL_DAMAGE_CHILD ? "CHILD " : "", */
            /*           damage() & FL_DAMAGE_ALL ? "ALL " : "", */
            /*           damage() & FL_DAMAGE_USER1 ? "USER 1 ": "", */
            /*           damage() & FL_DAMAGE_EXPOSE ? "EXPOSE " : "", */
            /*           damage() & FL_DAMAGE_SCROLL ? "SCROLL " : "", */
            /*           damage() & FL_DAMAGE_OVERLAY ? "OVERLAY " : ""); */

            if ( ! backbuffer ||
                 ! ( damage() & FL_DAMAGE_USER1 ) )
            {
                if ( !backbuffer )
                    backbuffer = fl_create_offscreen( W, H );

                DMESSAGE( "redrawing preview" );

                fl_begin_offscreen(backbuffer);
         
                fl_rectf( 0, 0, W, H, color() );
                canvas->m.grid->draw_notes( draw_dash, this );

                fl_end_offscreen();
            }

            fl_copy_offscreen( X,Y,W,H,backbuffer,0, 0 );
        }
public:

    void resize ( int X, int Y, int W, int H )
        {
            Fl_Panzoomer::resize( X,Y,W,H );
            if ( backbuffer )
                fl_delete_offscreen( backbuffer );
            backbuffer = 0;
            redraw();
        }

    void draw_overlay ( void )
        {
            Canvas *c = canvas;

            double HS = (double)w() /(  _xmax - _xmin );

            tick_t new_x = c->grid()->x_to_ts( c->grid()->ts_to_x( c->grid()->index() ) );
            fl_color( fl_color_add_alpha( FL_RED, 100 ) );
            fl_line( x() + new_x * HS, y(), x() + new_x * HS, y() + h() );
        }

};

static note_properties *ghost_note = 0;

Canvas::Canvas ( int X, int Y, int W, int H, const char *L ) : Fl_Group( X,Y,W,H,L )
{
    _selection_mode = SELECT_NONE;
    _move_mode = false;

    { Fl_Box *o = new Fl_Box( X, Y, W, H - 75 );
        /* this is a dummy group where the canvas goes */
        Fl_Group::current()->resizable( o );
    }
    { Fl_Group *o = new Fl_Group( X, Y + H - 75, W, 75 );
        
        {
            Canvas_Panzoomer *o = new Canvas_Panzoomer( X, Y + H - 75, W - 14, 75 );
            o->canvas = this;
            o->box( FL_FLAT_BOX );
//        o->color(fl_color_average( FL_BLACK, FL_WHITE, 0.90 ));
            o->color( FL_BLACK );
//        o->color(FL_BACKGROUND_COLOR);
//        o->type( FL_HORIZONTAL );
            o->callback( cb_scroll, this );
            o->when( FL_WHEN_CHANGED );
            panzoomer = o;
        }
        
        {
            Fl_Slider *o = new Fl_Slider( X + W - 14, Y + H - panzoomer->h(), 14, panzoomer->h() );
            o->range( 1, 128 );
            o->step( 1 );
            o->type( FL_VERTICAL );
            o->tooltip( "Vertical Zoom" );
            o->callback( cb_scroll, this );
            vzoom = o;
        }
        o->end();
    }

    m.origin_x = m.origin_y = m.height = m.width = m.div_w = m.div_h = m.margin_top = m.margin_left = m.playhead = m.w = m.h = m.p1 = m.p2 = m.p3 = m.p4 = 0;

    m.margin_top = ruler_height;

    m.draw = false;
    m.ruler_drawn = false;
    m.grid_drawn = false;

//    m.current = m.previous = NULL;

    m.row_compact = true;

    m.maxh = 128;

    m.vp = NULL;

    m.grid = NULL;

    end();

    resize( X,Y,W,H);
}

Canvas::~Canvas ( ) 
{

}

void
Canvas::handle_event_change ( void )
{
    /* mark the song as dirty and pass the signal on */
    song.set_dirty();

    Grid *g = grid();
    panzoomer->x_value( g->x_to_ts( m.vp->x), g->x_to_ts( m.vp->w ), 0, g->length());

    // panzoomer->redraw();

    redraw();
}

/** change grid to /g/, returns TRUE if new grid size differs from old */
void
Canvas::grid ( Grid *g )
{
    m.grid = g;

    if ( ! g )
        return;

    m.vp = &g->viewport;

    char *s = m.vp->dump();
    DMESSAGE( "viewport: %s", s );
    free( s );

    m.ruler_drawn = false;

    resize_grid();

    vzoom->range( 1, m.maxh );
    vzoom->value( m.vp->h );
    
    update_mapping();

    /* connect signals */
    /* FIXME: what happens when we do this twice? */
    g->signal_events_change.connect( mem_fun( this, &Canvas::handle_event_change ) );
    g->signal_settings_change.connect( signal_settings_change.make_slot() );
    
    redraw();

//     parent()->redraw();
    signal_settings_change();
}

/** keep row compaction tables up-to-date */
void
Canvas::_update_row_mapping ( void )
{
    /* reset */
    for ( int i = 128; i-- ; )
        m.rtn[i] = m.ntr[i] = -1;

    DMESSAGE( "updating row mapping" );

    /* rebuild */
    int r = 0;
    for ( int n = 0; n < 128; ++n )
    {
        if ( m.grid->row_name( n ) )
        {
            m.rtn[r] = n;
            m.ntr[n] = r;
            ++r;
        }
    }

    if ( m.row_compact && r )
        m.maxh = r;
    else
        m.maxh = 128;

    m.vp->h = min( m.vp->h, m.maxh );

    resize_grid();
}

/** update everything about mapping, leaving the viewport alone */
void
Canvas::update_mapping ( void )
{
    _update_row_mapping();

    adj_size();

//    int old_margin = m.margin_left;

    m.margin_left = 0;

    m.draw = false;

    m.grid->draw_row_names( this );

    m.draw = true;

/*     if ( m.margin_left != old_margin ) */
/*     { */
/* //        signal_resize(); */
/*         redraw(); */
/*     } */
/*     else */

    damage(FL_DAMAGE_USER1);

}

/** change grid mapping */
void
Canvas::changed_mapping ( void )
{
    update_mapping();

    m.vp->h = min( m.vp->h, m.maxh );

    if ( m.vp->y + m.vp->h > m.maxh )
        m.vp->y = (m.maxh / 2) - (m.vp->h / 2);
}

Grid *
Canvas::grid ( void )
{
    return m.grid;
}


/** recalculate node sizes based on physical dimensions */
void
Canvas::adj_size ( void )
{
    if ( ! m.vp )
        return;

    m.div_w = (m.width - m.margin_left) / m.vp->w;
    m.div_h = (m.height - m.margin_top) / m.vp->h;
}

/** reallocate buffers to match grid dimensions */
void
Canvas::resize_grid ( void )
{
    adj_size();
    
    DMESSAGE( "resizing grid %dx%d", m.vp->w, m.vp->h );

    Grid *g = grid();
    panzoomer->x_value( g->x_to_ts( m.vp->x), g->x_to_ts( m.vp->w ), 0, g->length());
    panzoomer->y_value( m.vp->y, m.vp->h, 0, m.maxh  );

    panzoomer->zoom_range( 2, 16 );

//    m.vp->w = max( 32, min( (int)(m.vp->w * n), 256 ) );

}

/** inform the canvas with new phsyical dimensions */
void
Canvas::resize ( int x, int y, int w, int h )
{
    m.origin_x = x;
    m.origin_y = y;

    m.width = w;
    m.height = h - 75;
    
    Fl_Group::resize(x,y,w,h);

    adj_size();
}



/***********/
/* Drawing */
/***********/

/** is /x/ within the viewport? */
bool
Canvas::viewable_x ( int x )
{
    return x >= m.vp->x && x < m.vp->x + m.vp->w;
}

static
int
gui_draw_ruler ( int x, int y, int w, int div_w, int div, int ofs, int p1, int p2 )
{
    /* Across the top */

   
    fl_font( FL_TIMES, ruler_height );

    int h = ruler_height;

    fl_color( FL_BACKGROUND_COLOR );

    w += 100;            /* FIXME: hack */

    //  fl_rectf( x, y, x + (div_w * w), y + h );
    fl_rectf( x, y, (div_w * w), h );


    fl_color( FL_FOREGROUND_COLOR );

    fl_line( x + div_w / 2, y, x + div_w * w, y );

    char pat[40];
    int z = div;
    int i;
    for ( i = 0; i < w; i++ )
    {
        int k = ofs + i;
        if ( 0 == k % z )
        {
            int nx = x + (i * div_w) + (div_w / 2);

            fl_color( FL_FOREGROUND_COLOR );

            fl_line( nx, y, nx, y + h - 1 );

            sprintf( pat, "%i", 1 + (k / z ));

            fl_color( FL_FOREGROUND_COLOR );
            fl_draw( pat, nx + div_w / 2, y + h + 1 / 2 );
        }
    }

    if ( p1 != p2 )
    {
        if ( p1 >= 0 )
        {
            if ( p1 < p2 )
                fl_color( fl_color_add_alpha( FL_GREEN, 100 ) );
            else
                fl_color( fl_color_add_alpha( FL_GREEN, 100 ) );

            fl_rectf( x + (div_w * p1), y + h / 2, div_w, h / 2 );

        }
        if ( p2 >= 0 )
        {
            if ( p2 < p1 )
                fl_color( fl_color_add_alpha( FL_GREEN, 100 ) );
            else
                fl_color( fl_color_add_alpha( FL_RED, 100 ) );
            fl_rectf( x + (div_w * p2), y + h / 2, div_w, h / 2 );

        }
    }

    return h;
}

static
int
gui_draw_string ( int x, int y, int w, int h, int color, const char *s, bool draw )
{
    int rw;

    if ( ! s )
        return 0;

    fl_font( FL_COURIER, min( h, 18 ) );

    rw = fl_width( s );

    if ( fl_not_clipped( x, y, rw, h ) && draw )
    {
//        fl_rectf( x,y,w,h, FL_BACKGROUND_COLOR );

        if ( color )
            fl_color( velocity_colors[ color ] );
        else
            fl_color( FL_DARK_CYAN );

        fl_draw( s, x, y + h / 2 + fl_descent() );
    }

    return rw;
}

/** callback called by Grid::draw_row_names() to draw an individual row name  */
void
Canvas::draw_row_name ( int y, const char *name, int color )
{
    bool draw = m.draw;

    y = ntr( y );

    y -= m.vp->y;

    int bx = m.origin_x;
    int by = m.origin_y + m.margin_top + y * m.div_h;
    int bw = m.margin_left;
    int bh = m.div_h;

    if ( y < 0 || y >= m.vp->h )
        draw = false;

    if ( draw && name )
    {
        fl_rectf( bx, by, bw, bh, index(name, '#') ? FL_GRAY : FL_BLACK );
        fl_rect( bx, by, bw, bh, FL_BLACK );
    }
    
    m.margin_left = max( m.margin_left, gui_draw_string( bx + 1, by + 2,
                                                         bw - 1, bh - 4,
                                                         color,
                                                         name,
                                                         draw ) );    
}

void
Canvas::draw_mapping ( void )
{
//    int old_margin = m.margin_left;

    m.margin_left = 0;

    m.draw = false;

    m.grid->draw_row_names( this );

    adj_size();

    m.draw = true;

    m.grid->draw_row_names( this );
}

void
Canvas::draw_ruler ( void )
{
    m.margin_top = gui_draw_ruler( m.origin_x + m.margin_left, 
                                   m.origin_y, 
                                   m.vp->w,
                                   m.div_w,
                                   m.grid->division(),
                                   m.vp->x,
                                   m.p1 - m.vp->x,
                                   m.p2 - m.vp->x );
}

void
Canvas::damage_grid ( tick_t x, int y, tick_t w, int h = 1 )
{
    y = ntr( y );

    if ( y < 0 )
        return;
   
    // adjust for viewport.
   
    x = m.grid->ts_to_x(x);
    w = m.grid->ts_to_x(w);
   
    x -= m.vp->x;
    y -= m.vp->y;

    if ( x < 0 || y < 0 || x >= m.vp->w || y >= m.vp->h )
        return;
   
    damage(FL_DAMAGE_USER1, m.origin_x + m.margin_left + x * m.div_w,
           m.origin_y + m.margin_top + y * m.div_h,
           m.div_w * w,
           m.div_h * h );
}

void
Canvas::draw_dash ( tick_t x, int y, tick_t w, int color, int selected ) const
{
    if ( m.grid->velocity_sensitive() )
        color = velocity_colors[ color ];
    else
        color = velocity_colors[ 127 ];

    y = ntr( y );
    
    if ( y < 0 )
        return;

    // adjust for viewport.

    x = m.grid->ts_to_x(x);
    w = m.grid->ts_to_x(w);

    x -= m.vp->x;
    y -= m.vp->y;

    x = m.origin_x + m.margin_left + x * m.div_w;
    y = m.origin_y + m.margin_top + y * m.div_h;
    w *= m.div_w;

    /* fl_rectf( x, y + 1, w, m.div_h - 1, fl_color_add_alpha( color, 170 ) ); */

    /* fl_rect( x, y + 1, w, m.div_h - 1, selected ? FL_MAGENTA : fl_lighter( FL_BACKGROUND_COLOR )); */
    
    if ( w > 4 )
    {
        fl_draw_box( FL_ROUNDED_BOX, x, y + 1, w, m.div_h - 1, color );
        
        if ( selected )
        {
            cairo_set_operator( Fl::cairo_cc(),CAIRO_OPERATOR_HSL_COLOR );

            fl_draw_box( FL_ROUNDED_BOX, x, y + 1, w, m.div_h - 1, FL_MAGENTA );

            cairo_set_operator( Fl::cairo_cc(),CAIRO_OPERATOR_OVER);
        }      
  /* if ( selected ) */
        /*     fl_draw_box( FL_ROUNDED_FRAME, x, y + 1, w, m.div_h - 1, FL_MAGENTA ); */
    }

// fl_color_add_alpha( color, 170 ));
}

/** callback used by Grid::draw()  */
void
Canvas::draw_dash ( tick_t x, int y, tick_t w, int color, int selected, void *userdata )
{
    Canvas *o = (Canvas*)userdata;
    
    o->draw_dash( x,y,w,color,selected );
}

int
Canvas::playhead_moved ( void )
{
    int x = m.grid->ts_to_x( m.grid->index() );
    
    return m.playhead != x;
}

void
Canvas::redraw_playhead ( void )
{
    int old_x = m.playhead;
    
    int new_x = m.grid->ts_to_x( m.grid->index() );

    if ( old_x != new_x )
    {
        window()->damage( FL_DAMAGE_OVERLAY );
    }

    if ( m.playhead < m.vp->x || m.playhead >= m.vp->x + m.vp->w )
    {
        if ( config.follow_playhead )
        {
            new_x = m.playhead;

            panzoomer->x_value( m.grid->index() );
            panzoomer->do_callback();
        }
    }
}

void
Canvas::draw_overlay ( void )
{
    if ( ! visible_r() )
        return;

    /* fl_push_no_clip(); */
    
    fl_push_clip( x() + m.margin_left,
                  y() + m.margin_top,
                  w() - m.margin_left,
                  h() - panzoomer->h() - m.margin_top );

    draw_playhead();

    if ( _selection_mode )
    {
        int X,Y,W,H;
        
        SelectionRect &s = _selection_rect;

        X = s.x1 < s.x2 ? s.x1 : s.x2;
        Y = s.y1 < s.y2 ? s.y1 : s.y2;
        W = s.x1 < s.x2 ? s.x2 - s.x1 : s.x1 - s.x2;
        H = s.y1 < s.y2 ? s.y2 - s.y1 : s.y1 - s.y2;
               
        /* fl_rectf( X,Y,W,H, fl_color_add_alpha( FL_MAGENTA, 50 ) ); */
        
        fl_rect( X,Y,W,H, FL_MAGENTA );
        
    }

    fl_pop_clip();

    panzoomer->draw_overlay();

    /* fl_pop_clip(); */

}

/** draw only the playhead--without reexamining the grid */
void
Canvas::draw_playhead ( void )
{
    int x = m.grid->ts_to_x( m.grid->index() );

    /* if ( m.playhead == x ) */
    /*     return; */

    m.playhead = x;

    /* if ( m.playhead < m.vp->x || m.playhead >= m.vp->x + m.vp->w ) */
    /*     return; */

    int px = m.origin_x + m.margin_left + ( x - m.vp->x ) * m.div_w;

    int X,Y,W,H;
 
    X = px;
    Y = m.origin_y + m.margin_top;
    W = m.div_w;
    H = m.origin_y + m.margin_top + m.vp->h * m.div_h;

    cairo_set_operator( Fl::cairo_cc(), CAIRO_OPERATOR_HSL_COLOR );

    fl_rectf( X,Y,W,H, FL_RED );

    cairo_set_operator( Fl::cairo_cc(), CAIRO_OPERATOR_OVER );

    fl_rect( X,Y,W,H, FL_RED );
}

void
Canvas::draw_clip ( void *v, int X, int Y, int W, int H )
{
    ((Canvas*)v)->draw_clip( X,Y,W,H );
}

void
Canvas::draw_clip ( int X, int Y, int W, int H )
{
    box( FL_FLAT_BOX );
    labeltype( FL_NO_LABEL );

    fl_push_clip( X,Y,W,H );


    fl_push_clip( m.origin_x + m.margin_left,
                  m.origin_y + m.margin_top,
                  w() - m.margin_left,
                  h() - m.margin_top - panzoomer->h() );

    fl_rectf( m.origin_x + m.margin_left, m.origin_y + m.margin_top, w(), h(), FL_BLACK );

    /* draw bar/beat lines */

    for ( int gx = m.vp->x;
          gx < m.vp->x + m.vp->w + 200;                                   /* hack */
          gx++ )
    {
        if ( m.grid->x_to_ts( gx ) > m.grid->length() )
            continue;

        if ( gx % m.grid->division() == 0 )
            fl_color( fl_color_average( FL_GRAY, FL_BLACK, 0.80 ) );
        else if ( gx % m.grid->subdivision() == 0 )
            fl_color( fl_color_average( FL_GRAY, FL_BLACK, 0.40 ) );
        else
            continue;
        
        fl_rectf( m.origin_x + m.margin_left + ( ( gx - m.vp->x ) * m.div_w ),
                  m.origin_y + m.margin_top, 
                  m.div_w, 
                  y() + h() - m.margin_top );
    }

    m.grid->draw_notes( draw_dash, this );

    if ( ghost_note )
        draw_dash( ghost_note->start,
                   ghost_note->note,
                   ghost_note->duration,
                   ghost_note->velocity,
                   1 );

    fl_color( fl_color_add_alpha( fl_rgb_color( 127,127,127 ), 50 ));

    /* draw grid */
    
    fl_begin_line();
    
    if ( m.div_w > 4 )
    {
        for ( int gx = m.origin_x + m.margin_left;
              gx < x() + w();
              gx += m.div_w )
        {
//            fl_line(  gx, m.origin_y + m.margin_top, gx, y() + h() );
            fl_vertex( gx, m.origin_y + m.margin_top );
            fl_vertex( gx, y() + h() );
            fl_gap();
        }
    }

    if ( m.div_h > 2 )
    {
        for ( int gy = m.origin_y + m.margin_top;
              gy < y() + h();
              gy += m.div_h )
        {
//            fl_line( m.origin_x + m.margin_left, gy,  x() + w(), gy ); 
            fl_vertex( m.origin_x + m.margin_left, gy );
            fl_vertex( x() + w(), gy );
            fl_gap();
        }
    }

    fl_end_line();

//done:
    fl_pop_clip();

    fl_pop_clip();
}


/** draw ONLY those nodes necessary to bring the canvas up-to-date with the grid */
void
Canvas::draw ( void )
{
    box( FL_NO_BOX );
    labeltype( FL_NO_LABEL );

    /* DMESSAGE( "%s%s%s%s%s%s", */
    /*           damage() & FL_DAMAGE_CHILD ? "CHILD " : "", */
    /*           damage() & FL_DAMAGE_ALL ? "ALL " : "", */
    /*           damage() & FL_DAMAGE_USER1 ? "USER 1 ": "", */
    /*           damage() & FL_DAMAGE_EXPOSE ? "EXPOSE " : "", */
    /*           damage() & FL_DAMAGE_SCROLL ? "SCROLL " : "", */
    /*           damage() & FL_DAMAGE_OVERLAY ? "OVERLAY " : ""); */


    if ( damage() & FL_DAMAGE_SCROLL )
    {
        draw_ruler();

        int dx = ( _old_scroll_x - m.vp->x ) * m.div_w;
        int dy = ( _old_scroll_y - m.vp->y ) * m.div_h;

        fl_scroll( m.origin_x + m.margin_left,
                   m.origin_y + m.margin_top,
                   w() - m.margin_left,
                   h() - m.margin_top - panzoomer->h(),
                   dx, dy, 
                   draw_clip,
                   this );

        if ( dy )
            draw_mapping();

        _old_scroll_x = m.vp->x;
        _old_scroll_y = m.vp->y;

        if ( damage() & FL_DAMAGE_CHILD )
            clear_damage( FL_DAMAGE_CHILD );
    }
    else if ( damage() & ~FL_DAMAGE_CHILD )
    {
        draw_mapping();
        draw_ruler();

        draw_clip( x(), y(), w(), h() );
    }

    draw_children();
}

void
Canvas::cb_scroll ( Fl_Widget *w, void *v )
{
    ((Canvas*)v)->cb_scroll( w );
}

void
Canvas::cb_scroll ( Fl_Widget *w )
{
    if ( w == panzoomer )
    {
        Fl_Panzoomer *o = (Fl_Panzoomer*)w;

        _old_scroll_x = m.vp->x;
        _old_scroll_y = m.vp->y;

        m.vp->x = grid()->ts_to_x( o->x_value() );
        m.vp->y = o->y_value();

        if ( m.vp->x != _old_scroll_x || m.vp->y != _old_scroll_y )
            damage( FL_DAMAGE_SCROLL );

        if ( o->zoom_changed() )
        {
            m.vp->w = m.grid->division() * o->zoom();
            resize_grid();
            redraw();
        }
    }
    else if ( w == vzoom )
    {
        Fl_Slider *o = (Fl_Slider*)w;
        
        float n = o->value();

        m.vp->h = min( (int)n, m.maxh );

        resize_grid();
        
        song.set_dirty();

        redraw();
    }

}


/** convert pixel coords into grid coords. returns true if valid */
bool
Canvas::grid_pos ( int *x, int *y ) const
{
    /* if ( ( *x < m.origin_x + m.margin_left ) || */
    /*        ( *y < m.origin_y + m.margin_top ) || */
    /*          ( *x > m.origin_x + w() ) || */
    /*        (*y > m.origin_y + h() - panzoomer->h() ) ) */
    /*     return false; */

    *y = (*y - m.margin_top - m.origin_y) / m.div_h;
    *x = (*x - m.margin_left - m.origin_x) / m.div_w;

    /* if ( *x < 0 || *y < 0 || *x >= m.vp->w || *y >= m.vp->h ) */
    /*     return false; */

    /* adjust for viewport */
    *x += m.vp->x;
    *y += m.vp->y;

    /* adjust for row-compaction */
    *y = rtn( *y );

    return true;
}



/******************/
/* Input handlers */
/******************/

/* These methods translate viewport pixel coords to absolute grid
   coords and pass on to the grid. */

/** if coords correspond to a row name entry, return the (absolute) note number, otherwise return -1 */
int
Canvas::is_row_press ( void ) const
{
    if ( Fl::event_inside( this->x(),
                           this->y() + this->m.margin_top,
                           this->m.margin_left,
                           ( this->h() - this->m.margin_top ) - this->panzoomer->h() ) )
    {
        int dx,dy;
        dx = Fl::event_x();
        dy = Fl::event_y();

        grid_pos( &dx, &dy );

        return m.grid->y_to_note(dy );
    }
    else
        return -1;
}

bool
Canvas::is_ruler_click ( void ) const
{
    return Fl::event_y() < m.origin_y + m.margin_top;
}

void
Canvas::start_cursor ( int x, int y )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.ruler_drawn = false;

    m.p1 = x;


    /* m.p3 = ntr( y ); */

    _lr();

    redraw();
}

void
Canvas::end_cursor ( int x, int y )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.ruler_drawn = false;

    m.p2 = x;

    /* m.p4 = ntr( y ); */

    _lr();

    redraw();
}

void
Canvas::adj_color ( int x, int y, int n )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.grid->adj_velocity( x, y, n );
}

void
Canvas::adj_length ( int x, int y, int n )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.grid->adj_duration( x, y, n );
}

void
Canvas::set_end ( int x, int y, int n )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.grid->set_end( x, y, n );
}

void
Canvas::select ( int x, int y )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.grid->toggle_select( x, y );
}

void
Canvas::move_selected ( int dir, int n )
{
    switch ( dir )
    {
        case RIGHT:
            m.grid->nudge_selected( n );
            break;
        case LEFT:
            m.grid->nudge_selected( 0 - n );
            break;
        case UP:
        case DOWN:
        {
            /* row-compaction makes this a little complicated */
            event_list *el = m.grid->events();

            /* FIXME: don't allow movement beyond the edges!  */

/*             int hi, lo; */

/*             m.grid->selected_hi_lo_note( &hi, &lo ); */

/*             hi = ntr( hi ) > 0 ? ntr( hi ) :  */

/*             if ( m.grid->y_to_note( ntr( hi ) ) ) */


            if ( dir == UP )
                for ( int y = 0; y <= m.maxh; ++y )
                    el->rewrite_selected( m.grid->y_to_note( rtn( y ) ), m.grid->y_to_note( rtn( y - n ) ) );
            else
                for ( int y = m.maxh; y >= 0; --y )
                    el->rewrite_selected( m.grid->y_to_note( rtn( y ) ), m.grid->y_to_note( rtn( y + n ) ) );

            m.grid->events( el );

            delete el;
            break;
        }
    }
}

void
Canvas::_lr ( void )
{
    int l, r;

    if ( m.p2 > m.p1 )
    {
        l = m.p1;
        r = m.p2;
    }
    else
    {
        l = m.p2;
        r = m.p1;
    }

    m.p1 = l;
    m.p2 = r;
}

void
Canvas::select_range ( void )
{
    if ( m.p3 == m.p4 )
        m.grid->select( m.p1, m.p2 );
    else
        m.grid->select( m.p1, m.p2, rtn( m.p3 ), rtn( m.p4 ) );
}

void
Canvas::invert_selection ( void )
{
    m.grid->invert_selection();
}

void
Canvas::crop ( void )
{
    if ( m.p3 == m.p4 )
        m.grid->crop( m.p1, m.p2 );
    else
        m.grid->crop( m.p1, m.p2, rtn( m.p3 ), rtn( m.p4 ) );

    m.vp->x = 0;

    m.p2 = m.p2 - m.p1;
    m.p1 = 0;

    m.ruler_drawn = false;
}

void
Canvas::delete_time ( void )
{
    m.grid->delete_time( m.p1, m.p2 );
}


void
Canvas::insert_time ( void )
{
    m.grid->insert_time( m.p1, m.p2 );
}

/** paste range as new grid */
void
Canvas::duplicate_range ( void )
{
    Grid *g = m.grid->clone();

    g->crop( m.p1, m.p2 );
    g->viewport.x = 0;
}

void
Canvas::cut ( void )
{
    m.grid->cut();
}

void
Canvas::copy ( void )
{
    m.grid->copy();
}

void
Canvas::paste ( void ) 
{
    if ( m.p1 != m.p2 && m.p1 > m.vp->x && m.p1 < m.vp->x + m.vp->w )
        m.grid->paste( m.p1 );
    else
        m.grid->paste( m.vp->x );
}

void
Canvas::row_compact ( int n )
{
    switch ( n )
    {
        case OFF:
            m.row_compact = false;
            m.maxh = 128;
            break;
        case ON:
            m.row_compact = true;
            m.vp->y = 0;
            _update_row_mapping();
            break;
        case TOGGLE:
            row_compact( m.row_compact ? OFF : ON );
            break;
    }
//    _reset();
}

void
Canvas::pan ( int dir, int n )
{

    switch ( dir )
    {
        case LEFT: case RIGHT: case TO_PLAYHEAD: case TO_NEXT_NOTE: case TO_PREV_NOTE:
            /* handle horizontal movement specially */
            n *= m.grid->division();
            m.ruler_drawn = false;
            break;
        default:
            n *= 5;
            break;
    }

    switch ( dir )
    {
        case LEFT:
            m.vp->x = max( m.vp->x - n, 0 );
            break;
        case RIGHT:
            m.vp->x += n;
            break;
        case TO_PLAYHEAD:
            m.vp->x = m.playhead - (m.playhead % m.grid->division());
            break;
        case UP:
            m.vp->y = max( m.vp->y - n, 0 );
            break;
        case DOWN:
            m.vp->y = min( m.vp->y + n, m.maxh - m.vp->h );
            break;
        case TO_NEXT_NOTE:
        {
            int x = m.grid->next_note_x( m.vp->x );
            m.vp->x = x - (x % m.grid->division() );
            break;
        }
        case TO_PREV_NOTE:
        {
            int x = m.grid->prev_note_x( m.vp->x );
            m.vp->x = x - (x % m.grid->division() );
            break;
        }
    }

    damage(FL_DAMAGE_USER1);
}

void
Canvas::can_scroll ( int *left, int *right, int *up, int *down )
{
    *left = m.vp->x;
    *right = -1;
    *up = m.vp->y;
    *down = m.maxh - ( m.vp->y + m.vp->h );
}


/** adjust horizontal zoom (* n) */
void
Canvas::h_zoom ( float n )
{
    m.vp->w = max( 32, min( (int)(m.vp->w * n), 256 ) );

    resize_grid();

    song.set_dirty();
}

void
Canvas::selected_velocity ( int v )
{
    grid()->selected_velocity( v );
}

void
Canvas::v_zoom_fit ( void )
{
    if ( ! m.grid )
        return;

    changed_mapping();

    m.vp->h = m.maxh;
    m.vp->y = 0;

    resize_grid();

    song.set_dirty();

}

/** adjust vertical zoom (* n) */
void
Canvas::v_zoom ( float n )
{
    m.vp->h = max( 1, min( (int)(m.vp->h * n), m.maxh ) );

    resize_grid();

    song.set_dirty();
}

void
Canvas::notes ( char *s )
{
    m.grid->notes( s );
}

char *
Canvas::notes ( void )
{
    return m.grid->notes();
}


int
Canvas::handle ( int m )
{
    Canvas *c = this;

    static int last_move_x = 0;
    static int last_move_y = 0;

//    static bool range_select;

    int x, y;
    int processed = 1;

    x = Fl::event_x();
    y = Fl::event_y();

    static int drag_x;
    static int drag_y;
    static bool delete_note;
    static note_properties *drag_note;

    switch ( m )
    {
        case FL_FOCUS:
        case FL_UNFOCUS:
            damage( FL_DAMAGE_ALL );
            return 1;
        case FL_ENTER:
        case FL_LEAVE:
            fl_cursor( FL_CURSOR_DEFAULT );
            return 1;
        case FL_MOVE:
        {
            if ( Fl::event_inside( this->x() + this->m.margin_left,
                                   this->y() + this->m.margin_top,
                                   this->w() - this->m.margin_left,
                                   ( this->h() - this->m.margin_top ) - this->panzoomer->h() ) )
                fl_cursor( FL_CURSOR_HAND );
            else
                fl_cursor( FL_CURSOR_DEFAULT );

            return 1;
            break;
        }
        case FL_KEYBOARD:
        {
      
/*             if ( Fl::event_state() & FL_ALT || Fl::event_state() & FL_CTRL ) */
/*                 // this is more than a simple keypress. */
/*                 return 0; */

            if ( Fl::event_state() & FL_CTRL )
            {
                switch ( Fl::event_key() )
                {
                    case FL_Delete:
                        c->delete_time();
                        break;
                    case FL_Insert:
                        c->insert_time();
                        break;
                    case FL_Right:
                        c->pan( TO_NEXT_NOTE, 0 );
                        break;
                    case FL_Left:
                        c->pan( TO_PREV_NOTE, 0 );
                        break;
                    default:
                        return 0;
                }
            }
            else
                if ( Fl::event_state() & FL_ALT )
                    return 0;

            switch ( Fl::event_key() )
            {
                case FL_Left:
                    c->pan( LEFT, 1 );
                    break;
                case FL_Right:
                    c->pan( RIGHT, 1 );
                    break;
                case FL_Up:
                    c->pan( UP, 1 );
                    break;
                case FL_Down:
                    c->pan( DOWN, 1 );
                    break;
                default:
                    /* have to do this to get shifted keys */
                    switch ( *Fl::event_text() )
                    {
                        case 'f':
                            c->pan( TO_PLAYHEAD, 0 );
                            break;
                        case 'r':
                            c->select_range();
                            break;
                        case 'q':
                            c->grid()->select_none();
                            break;
                        case 'i':
                            c->invert_selection();
                            break;
                            /* case '1': */
                            /*     c->h_zoom( 2.0f ); */
                            /*     break; */
                            /* case '2': */
                            /*     c->h_zoom( 0.5f ); */
                            /*     break; */
                            /* case '3': */
                            /*     c->v_zoom( 2.0f ); */
                            /*     break; */
                            /* case '4': */
                            /*     c->v_zoom( 0.5f ); */
                            /*     break; */
                            /* case ' ': */
                            /*     transport.toggle(); */
                            /*     break; */

#define IS_PATTERN (parent() == ui->pattern_tab)
#define IS_PHRASE (parent() == ui->phrase_tab)
#define IS_SEQUENCE (parent() == ui->sequence_tab)
                        case '<':
                            c->move_selected( LEFT, 1 );
                            break;
                        case '>':
                            c->move_selected( RIGHT, 1 );
                            break;
                        case ',':
                            c->move_selected( UP, 1 );
                            break;
                        case '.':
                            c->move_selected( DOWN, 1 );
                            break;
                        case 'C':
                            c->crop();
                            break;
                        case 'd':
                        {
                            MESSAGE( "duplicating thing" );
                            c->grid( c->grid()->clone() );

                            // number of phrases may have changed.
                            ui->update_sequence_widgets();

                            break;

                        }
                        case 'D':
                            c->duplicate_range();
                            break;
                        case 't':
                            c->grid()->trim();
                            break;
                        default:
                            processed = 0;
                            break;
                    }
                    break;
            }
            break;
        }
        case FL_PUSH:
        {
            Fl::focus(this);

            switch ( Fl::event_button() )
            {
                case 1:
                {
                    if ( is_ruler_click() )
                    {
                        c->start_cursor( x, y );
                        //           return 1;
                        _selection_mode = SELECT_RANGE;
                       
                    }

                    if ( _selection_mode )
                    {
                        drag_x = Fl::event_x();
                        drag_y = Fl::event_y();
                    
                        _selection_rect.x1 = drag_x;
                        _selection_rect.y1 = drag_y;
                        _selection_rect.x2 = drag_x;
                        _selection_rect.y2 = drag_y;

                        if ( _selection_mode == SELECT_RANGE )
                        {
                            _selection_rect.y1 = 0;
                            _selection_rect.y2 = 2000;
                        }

                        return 1;
                    }

                    delete_note = true;

                    if ( Fl::event_ctrl() )
                    {
                        c->select( x, y );
                        processed = 2;
                        break;
                    }

                    int dx = x;
                    int dy = y;
                    
                    grid_pos( &dx, &dy );

                    int note;
                    if ( ( note = c->is_row_press() ) >= 0 )
                    {
                        if ( IS_PATTERN )
                            ((pattern *)c->grid())->row_name_press( note );
                        
                        processed = 2;
                        break;
                    }

                    if ( Fl::event_inside( this->x() + this->m.margin_left,
                                           this->y() + this->m.margin_top,
                                           this->w() - this->m.margin_left,
                                           ( this->h() - this->m.margin_top ) - this->panzoomer->h() ) )
                    {

                        if ( ! this->m.grid->is_set( dx,dy ))
                        {
                            ghost_note = new note_properties;
                            drag_note = new note_properties;

                            ghost_note->start = this->m.grid->x_to_ts( dx );
                            ghost_note->note = dy;
                            ghost_note->duration = this->m.grid->default_length();
                            ghost_note->velocity = 64;

                            drag_note->start = this->m.grid->x_to_ts( dx );
                            drag_note->note = dy;
                            drag_note->duration = this->m.grid->default_length();
                            drag_note->velocity = 64;
                    

                            
                            delete_note = false;

                            processed = 1;
                            break;
                        }
                        else
                        {
                            note_properties np;
                            this->m.grid->get_note_properties( dx, dy, &np );

                            if ( np.selected )
                            {
                                _move_mode = true;
                                /* initiate move */
                                last_move_x = dx;
                                last_move_y = ntr( dy );
                            }
                            else
                            {
                                ghost_note = new note_properties;
                                drag_note = new note_properties;
                                this->m.grid->get_note_properties( dx, dy, ghost_note );
                                this->m.grid->get_note_properties( dx, dy, drag_note );
                                this->m.grid->del( dx, dy );
                                
                                delete_note = true;
                            }
                        }

                        this->m.grid->get_start( &dx, &dy );                    

                        drag_x = x;
                        drag_y = y;
                    
                        take_focus();
                    }
                    else
                        processed = 0;

                    break;
                }
                case 3:
                {
                    int note;
                    if ( ( note = is_row_press() ) >= 0 )
                    {
                        /* inside the note headings */

                        DMESSAGE( "click on row %d", note );
                        if ( IS_PATTERN )
                        {
                            Instrument *i = ((pattern *)c->grid())->mapping.instrument();

                            if ( i )
                            {
                                ui->edit_instrument_row( i, note );

                                c->changed_mapping();
                            }
                        }
                    }
                    else
                    {
                        _selection_mode = SELECT_RECTANGLE;
                        {
                            drag_x = Fl::event_x();
                            drag_y = Fl::event_y();
                        
                            _selection_rect.x1 = drag_x;
                            _selection_rect.y1 = drag_y;
                            _selection_rect.x2 = drag_x;
                            _selection_rect.y2 = drag_y;
                            signal_settings_change();
                            return 1;
                        }
                        
                        return 1;
                        break;

                  
                    }

                    /* if ( Fl::event_state() & FL_SHIFT ) */
                    /* { */
                    /*     c->end_cursor( x, y ); */
                    /*     break; */
                    /* } */
                    break;
                }
                default:
                    processed = 0;
            }
            break;
        }
        case FL_RELEASE:
        {
            _move_mode = false;
           
            if ( SELECT_RANGE == _selection_mode )
            {
                select_range();
                _selection_mode = SELECT_NONE;
                return 1;
            }
            else if ( SELECT_RECTANGLE == _selection_mode )
            {
                int X,Y,W,H;
                
                SelectionRect &s = _selection_rect;
        
                X = s.x1 < s.x2 ? s.x1 : s.x2;
                Y = s.y1 < s.y2 ? s.y1 : s.y2;
                W = s.x1 < s.x2 ? s.x2 - s.x1 : s.x1 - s.x2;
                H = s.y1 < s.y2 ? s.y2 - s.y1 : s.y1 - s.y2;
                
                int endx = X + W;
                int endy = Y + H;
                int beginx = X;
                int beginy = Y;
                
                grid_pos( &beginx, &beginy );
                grid_pos( &endx, &endy );
               
                /* if ( is_ruler_click() ) */
                /* { */
                /*     grid()->select( beginx, endx ); */
                /* } */
                /* else */
                {
                    grid()->select( beginx, endx, beginy, endy );
                }

                _selection_mode = SELECT_NONE;

                _selection_rect.x1 = 0;
                _selection_rect.y1 = 0;
                _selection_rect.x2 = 0;
                _selection_rect.y2 = 0;

                damage(FL_DAMAGE_OVERLAY);
                signal_settings_change();
                return 1;
            }

            int dx = x;
            int dy = y;
            grid_pos( &dx, &dy );

                if ( delete_note )
                {
//                            this->m.grid->del( dx, dy );
                    if ( ghost_note )
                    {
                        damage_grid( ghost_note->start, ghost_note->note, ghost_note->duration, 1 );

                        delete ghost_note;
                    }
                    ghost_note = 0;
                }
                else
                    if ( ghost_note )
                    {
                        this->m.grid->put( this->m.grid->ts_to_x( ghost_note->start ), 
                                           ghost_note->note,
                                           ghost_note->duration,
                                           ghost_note->velocity);

                        delete_note = false;
                                
                        delete ghost_note;
                        ghost_note = 0;
                    }

            if ( drag_note )
                delete drag_note;
            drag_note = 0;

            grid()->select_none();

            break;
        }
         
        case FL_DRAG:

            if ( Fl::event_is_click() )
                return 1;

            {
                if ( _selection_mode )
                {
                    grid()->select_none();

                    _selection_rect.x2 = x;
                    _selection_rect.y2 = y;

                    if ( SELECT_RANGE == _selection_mode )
                    {
                        _selection_rect.y2 = 2000;
                        c->end_cursor( x, y );
                    }

                    damage(FL_DAMAGE_OVERLAY);
                
                    return 1;
                }


                int dx = x;
                int dy = y;

                grid_pos( &dx, &dy );

                if ( _move_mode )
                { 
                    int odx = drag_x;
                    int ody = drag_y;
                    grid_pos( &odx, &ody );
               
                    if ( last_move_x != dx )
                    {
                        //this->m.grid->move_selected( dx - move_xoffset );
                        if ( dx > last_move_x )
                            move_selected( RIGHT, dx - last_move_x );
                        else
                            move_selected( LEFT, last_move_x - dx );
                    }
    
                    dy = ntr( dy );

                    if ( dy != last_move_y )
                    {
                        if ( dy > last_move_y  )
                            move_selected( DOWN, dy - last_move_y );
                        else
                            move_selected( UP, last_move_y - dy );
                    }
                
                    last_move_y = dy;
                    last_move_x = dx;
                    return 1;
                }
      
                if ( ghost_note )
                {
                    damage_grid( ghost_note->start, ghost_note->note, ghost_note->duration, 1 );

                    int ody = drag_y;
                    int odx = drag_x;
                    
                    if ( drag_note )
                    {
                        grid_pos( &odx, &ody );
                        
                        /* cursor must leave the row to begin adjusting velocity. */
                        if ( ody != dy )
                        {
                            ghost_note->velocity =
                                drag_note->velocity +
                                ( (drag_y - y) / 3.0f );
                            
                            if ( ghost_note->velocity < 0 )
                                ghost_note->velocity = 0;
                            else if ( ghost_note->velocity > 127 )
                                ghost_note->velocity = 127;
                        }
                    }

                    if ( dx != odx )
                        {
                            if ( dx > this->m.grid->ts_to_x( ghost_note->start ) )
                            {
                                ghost_note->duration = this->m.grid->x_to_ts( dx  ) - ghost_note->start;
                            }
                        }
                    
                        
                    damage_grid( ghost_note->start, ghost_note->note, ghost_note->duration, 1 );

                    delete_note = false;

                    processed = 2;
             
                }
            }
            break;
        default:
            processed = 0;
    }

    if ( processed )
        window()->damage(FL_DAMAGE_OVERLAY);

    if ( processed == 1 )
        damage(FL_DAMAGE_USER1);

    if ( ! processed )
        return Fl_Group::handle( m );
    
    return processed;
}
