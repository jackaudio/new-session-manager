
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

#include "canvas.H"
#include "pattern.H"
#include "gui/draw.H"
#include "common.h"

#include "non.H"

cell_t **
Canvas::_alloc_array ( void )
{
    cell_t **a;

    int one = sizeof( typeof( a ) ) * m.vp->w;
    int two = sizeof( typeof( a[0] ) ) * (m.vp->h * m.vp->w);

    a = (cell_t **) malloc( one + two );

    m.size = one + two;

    cell_t *c = (cell_t *) (((unsigned char *)a) + one);

    for ( uint x = m.vp->w; x-- ; )
    {
        a[x] = c;
        c += m.vp->h;
        for ( uint y = m.vp->h; y-- ; )
        {
            a[ x ][ y ].flags = 0;
            a[ x ][ y ].state = -1;
            a[ x ][ y ].shape = SQUARE;
            a[ x ][ y ].color = 0;
        }
    }

    m.w = m.vp->w;
    m.h = m.vp->h;

    return a;
}

Canvas::Canvas ( )
{
    m.origin_x = m.origin_y = m.height = m.width = m.div_w = m.div_h = m.playhead = m.margin_top = m.margin_left = m.playhead = m.w = m.h = m.p1 = m.p2 = m.p3 = m.p4 = 0;

    m.margin_top = ruler_height;

    m.draw = false;
    m.ruler_drawn = false;
    m.mapping_drawn = false;
    m.grid_drawn = false;

    m.current = m.previous = NULL;

    m.row_compact = true;

    m.maxh = 128;

    m.vp = NULL;
}

void
Canvas::handle_event_change ( void )
{
    /* mark the song as dirty and pass the signal on */
    song.dirty( true );

    signal_draw();
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
    DEBUG( "viewport: %s", s );
    free( s );

    m.ruler_drawn = false;

    resize_grid();

    changed_mapping();

    m.shape = m.grid->draw_shape();

    /* connect signals */
    /* FIXME: what happens when we do this twice? */
    g->signal_events_change.connect( mem_fun( this, &Canvas::handle_event_change ) );
    g->signal_settings_change.connect( signal_settings_change.make_slot() );

    signal_draw();
    signal_settings_change();
}

/** keep row compaction tables up-to-date */
void
Canvas::_update_row_mapping ( void )
{
    /* reset */
    for ( int i = 128; i-- ; )
        m.rtn[i] = m.ntr[i] = -1;

    DEBUG( "updating row mapping" );

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
}

/** change grid mapping */
void
Canvas::changed_mapping ( void )
{
    _update_row_mapping();

    m.mapping_drawn = false;

    m.vp->y = (m.maxh / 2) - (m.vp->h / 2);

    resize();

    int old_margin = m.margin_left;

    m.margin_left = 0;

    m.draw = false;

    m.grid->draw_row_names( this );

    if ( m.margin_left != old_margin )
    {
        signal_resize();
        signal_draw();
    }
    else
        signal_draw();
}

Grid *
Canvas::grid ( void )
{
    return m.grid;
}


/** recalculate node sizes based on physical dimensions */
void
Canvas::resize ( void )
{
    if ( ! m.vp )
        return;

    m.div_w = (m.width - m.margin_left) / m.vp->w;
    m.div_h = (m.height - m.margin_top) / m.vp->h;

    m.border_w = min( m.div_w, m.div_h ) / 8;

    m.mapping_drawn = m.ruler_drawn = false;
}

/** reallocate buffers to match grid dimensions */
void
Canvas::resize_grid ( void )
{
    //   _update_row_mapping();

    resize();

    if ( m.vp )
    {
        if ( m.vp->w != m.w || m.vp->h != m.h ||
            m.div_w != m.old_div_w || m.div_h != m.old_div_h )
        {
            if ( m.grid_drawn )
                signal_resize();

            m.old_div_w = m.div_w;
            m.old_div_h = m.div_h;
        }
        else
            return;
    }

    DEBUG( "resizing grid %dx%d", m.vp->w, m.vp->h );

    if ( m.previous )
    {
        free( m.previous );
        free( m.current );
    }

    m.current = _alloc_array();
    m.previous = _alloc_array();

    m.grid_drawn = false;
}

/** inform the canvas with new phsyical dimensions */
void
Canvas::resize ( int x, int y, int w, int h )
{
    m.origin_x = x;
    m.origin_y = y;

    m.width = w;
    m.height = h;

    resize();
}



/***********/
/* Drawing */
/***********/

/** copy last buffer into current */
void
Canvas::copy ( void )
{
    for ( uint y = m.vp->h; y-- ; )
        for ( uint x = m.vp->w; x-- ; )
            m.current[ x ][ y ] = m.previous[ x ][ y ];
}


/** reset last buffer */
void
Canvas::_reset ( void )
{
    cell_t empty = {0,0,0,0};

    for ( uint y = m.vp->h; y-- ; )
        for ( uint x = m.vp->w; x-- ; )
            m.current[ x ][ y ] = empty;
}

/** prepare current buffer for drawing (draw "background") */
void
Canvas::clear ( void )
{
    uint rule = m.grid->ppqn();

    uint lx = m.grid->ts_to_x( m.grid->length() );

    for ( uint y = m.vp->h; y--; )
        for ( uint x = m.vp->w; x--; )
        {
            m.current[ x ][ y ].color = 0;
            m.current[ x ][ y ].shape = m.shape;
            m.current[ x ][ y ].state = EMPTY;
            m.current[ x ][ y ].flags = 0;
        }

    for ( int x = m.vp->w - rule; x >= 0; x -= rule )
        for ( uint y = m.vp->h; y-- ; )
            m.current[ x ][ y ].state = LINE;

    int sx = (int)(lx - m.vp->x) >= 0 ? lx - m.vp->x : 0;

    for ( int x = sx; x < m.vp->w; ++x )
        for ( int y = m.vp->h; y-- ; )
            m.current[ x ][ y ].state = PARTIAL;

}

/** is /x/ within the viewport? */
bool
Canvas::viewable_x ( int x )
{
    return x >= m.vp->x && x < m.vp->x + m.vp->w;
}

/** flush delta of last and current buffers to screen, then flip them */
void
Canvas::flip ( void )
{
    /* FIXME: should this not go in clear()? */
    if ( m.p1 != m.p2 )
    {
        if ( viewable_x( m.p1 ) ) draw_line( m.p1 - m.vp->x, F_P1 );
        if ( viewable_x( m.p2 ) ) draw_line( m.p2 - m.vp->x, F_P2 );
    }

    if ( viewable_x( m.playhead ) ) draw_line( m.playhead - m.vp->x, F_PLAYHEAD );

    for ( uint y = m.vp->h; y--; )
        for ( uint x = m.vp->w; x--; )
        {
            cell_t *c = &m.current[ x ][ y ];
            cell_t *p = &m.previous[ x ][ y ];

            /* draw selection rect */
            if ( m.p3 != m.p4 )
                if ( y + m.vp->y >= m.p3 && x + m.vp->x >= m.p1 &&
                     y + m.vp->y < m.p4 && x + m.vp->x < m.p2 )
                    c->flags |= F_SELECTION;

            if ( *c != *p )
                gui_draw_shape( m.origin_x + m.margin_left + x * m.div_w, m.origin_y + m.margin_top + y * m.div_h, m.div_w, m.div_h, m.border_w,
                                c->shape, c->state, c->flags, c->color );
        }

    cell_t **tmp = m.previous;

    m.previous = m.current;
    m.current = tmp;
}

/** redraw the ruler at the top of the canvas */
void
Canvas::redraw_ruler ( void )
{
    m.margin_top = gui_draw_ruler( m.origin_x + m.margin_left, m.origin_y, m.vp->w, m.div_w, m.grid->division(), m.vp->x,
                                   m.p1 - m.vp->x, m.p2 - m.vp->x );
    m.ruler_drawn = true;
}

/** callback called by Grid::draw_row_names() to draw an individual row name  */
void
Canvas::draw_row_name ( int y, const char *name, int color )
{
    bool draw = m.draw;
    bool clear = false;

    y = ntr( y );

    if ( ! m.row_compact && ! name )
        clear = true;

    y -= m.vp->y;

    int bx = m.origin_x;
    int by = m.origin_y + m.margin_top + y * m.div_h;
    int bw = min( m.margin_left, m.width / 8 );
    int bh = m.div_h;

    if ( y < 0 || y >= m.vp->h )
        draw = false;

    if ( clear && draw )
        gui_clear_area( bx, by, bw, bh );
    else
        m.margin_left = max( m.margin_left, gui_draw_string( bx, by,
                                                             bw, bh,
                                                             color,
                                                             name,
                                                             draw ) );
}

/** redraw row names */
void
Canvas::redraw_mapping ( void )
{
    m.margin_left = 0;

    m.draw = false;

    m.grid->draw_row_names( this );

    resize();

    m.draw = true;

    m.grid->draw_row_names( this );

    m.mapping_drawn = true;
}

void
Canvas::draw_mapping ( void )
{
    if ( ! m.mapping_drawn ) redraw_mapping();
}

void
Canvas::draw_ruler ( void )
{
    if ( ! m.ruler_drawn ) redraw_ruler();
}

/** "draw" a shape in the backbuffer */
void
Canvas::draw_shape ( int x, int y, int shape, int state, int color, bool selected )
{
    y = ntr( y );

    if ( y < 0 )
        return;

    // adjust for viewport.

    x -= m.vp->x;
    y -= m.vp->y;

    if ( x < 0 || y < 0 || x >= m.vp->w || y >= m.vp->h )
        return;

    m.current[ x ][ y ].shape = shape;
    m.current[ x ][ y ].color = color;
    m.current[ x ][ y ].state = (uint)m.vp->x + x > m.grid->ts_to_x( m.grid->length() ) ? PARTIAL : state;
    if ( selected )
        m.current[ x ][ y ].state = SELECTED;
    m.current[ x ][ y ].flags = 0;
}

/** callback used by Grid::draw()  */
void
Canvas::draw_dash ( int x, int y, int l, int shape, int color, bool selected )
{
    draw_shape( x, y, shape, FULL, color, selected );
    for ( int i = x + l - 1; i > x; i-- )
    {
        draw_shape( i, y, shape, CONTINUED, 0, selected );
    }
}

/** draw a vertical line with flags */
void
Canvas::draw_line ( int x, int flags )
{
    for ( uint y = m.vp->h; y-- ; )
        m.current[ x ][ y ].flags |= flags;
}


/** draw only the playhead--without reexamining the grid */
int
Canvas::draw_playhead ( void )
{
    int x = m.grid->ts_to_x( m.grid->index() );

    if ( m.playhead == x )
        return 0;

    m.playhead = x;

    if ( m.playhead < m.vp->x || m.playhead >= m.vp->x + m.vp->w )
    {
        if ( config.follow_playhead )
        {
            m.vp->x = m.playhead / m.vp->w * m.vp->w;

            m.ruler_drawn = false;

            signal_draw();

            return 0;
        }
    }

    copy();

    for ( uint x = m.vp->w; x-- ; )
        for ( uint y = m.vp->h; y-- ; )
            m.current[ x ][ y ].flags &= ~ (F_PLAYHEAD | F_P1 | F_P2 );

    flip();

    return 1;
}

/** draw ONLY those nodes necessary to bring the canvas up-to-date with the grid */
void
Canvas::draw ( void )
{
    DEBUG( "drawing canvas" );

    draw_mapping();
    draw_ruler();

    m.grid_drawn = true;

    m.grid->draw( this, m.vp->x, m.vp->y, m.vp->w, m.vp->h );
}

/** redraw every node on the canvas from the buffer (without
 * necessarily reexamining the grid) */
void
Canvas::redraw ( void )
{
    DEBUG( "redrawing canvas" );

    if ( ! m.grid_drawn )
        draw();

    m.ruler_drawn = false;
    m.mapping_drawn = false;

    draw_mapping();
    draw_ruler();

    for ( int y = m.vp->h; y--; )
        for ( int x = m.vp->w; x--; )
        {
            cell_t c = m.previous[ x ][ y ];

            if ( c.shape > HEXAGON ) return;

            if ( m.vp->x + x == m.playhead )
                c.flags |= F_PLAYHEAD;

            gui_draw_shape( m.origin_x + m.margin_left + x * m.div_w, m.origin_y + m.margin_top + y * m.div_h, m.div_w, m.div_h, m.border_w,
                            c.shape, c.state, c.flags, c.color );
        }
}

/** convert pixel coords into grid coords. returns true if valid */
bool
Canvas::grid_pos ( int *x, int *y ) const
{
    *y = (*y - m.margin_top - m.origin_y) / m.div_h;
    *x = (*x - m.margin_left - m.origin_x) / m.div_w;

    if ( *x < 0 || *y < 0 || *x >= m.vp->w || *y >= m.vp->h )
        return false;

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
Canvas::is_row_name ( int x, int y )
{
    if ( x - m.origin_x >= m.margin_left )
        return -1;

    x = m.margin_left;

    grid_pos( &x, &y );

    return m.grid->y_to_note( y );
}

void
Canvas::start_cursor ( int x, int y )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.ruler_drawn = false;

    m.p1 = x;
    m.p3 = ntr( y );

    _lr();

    signal_draw();
}

void
Canvas::end_cursor ( int x, int y )
{
    if ( ! grid_pos( &x, &y ) )
        return;

    m.ruler_drawn = false;

    m.p2 = x;
    m.p4 = ntr( y );

    _lr();

    signal_draw();
}

void
Canvas::set ( int x, int y )
{
    if ( y - m.origin_y < m.margin_top )
        /* looks like a click on the ruler */
    {
        if ( x - m.margin_left - m.origin_x >= 0 )
        {
            m.p1 = m.vp->x + ((x - m.margin_left - m.origin_x) / m.div_w);
            m.ruler_drawn = false;

            m.p3 = m.p4 = 0;
        }

        _lr();

        signal_draw();

        return;
    }

    if ( ! grid_pos( &x, &y ) )
        return;

    m.grid->put( x, y, 0 );
}

void
Canvas::unset ( int x, int y )
{
    if ( y - m.origin_y < m.margin_top )
        /* looks like a click on the ruler */
    {
        if ( x - m.margin_left - m.origin_x >= 0 )
        {
            m.p2 = m.vp->x + ((x - m.margin_left - m.origin_x) / m.div_w);
            m.ruler_drawn = false;

            m.p3 = m.p4 = 0;
        }

        _lr();

        signal_draw();

        return;
    }

    if ( ! grid_pos( &x, &y ) )
        return;

    m.grid->del( x, y );
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
            m.grid->move_selected( n );
            break;
        case LEFT:
            m.grid->move_selected( 0 - n );
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
Canvas::randomize_row ( int y )
{
    int x = m.margin_left;

    if ( ! grid_pos( &x, &y ) )
        return;

    ((pattern*)m.grid)->randomize_row( y, song.random.feel, song.random.probability );
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
    m.grid->crop( m.p1, m.p2 );

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
    _reset();
    m.mapping_drawn = false;
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
            m.mapping_drawn = false;
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

    signal_draw();
}

/** adjust horizontal zoom (* n) */
void
Canvas::h_zoom ( float n )
{
    m.vp->w = max( 32, min( (int)(m.vp->w * n), 256 ) );

    resize_grid();
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
}

/** adjust vertical zoom (* n) */
void
Canvas::v_zoom ( float n )
{
    m.vp->h = max( 1, min( (int)(m.vp->h * n), m.maxh ) );

    resize_grid();
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
