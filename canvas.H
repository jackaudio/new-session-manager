
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

#pragma once

#include "grid.H"
#include "gui/draw.H"

#include <sigc++/sigc++.h>
using namespace sigc;

class Mapping;

struct cell_t {
    unsigned char color;
    unsigned char shape : 4;
    unsigned char state : 4;
    unsigned char flags : 4;

    bool
    operator!= ( const cell_t &rhs )
        {
            return color != rhs.color || shape != rhs.shape || state != rhs.state || flags != rhs.flags;
        }
};

enum { LEFT, RIGHT, UP, DOWN, TO_PLAYHEAD, TO_NEXT_NOTE, TO_PREV_NOTE };


class Canvas : public trackable
{

    struct {
        int origin_x, origin_y;
        int width, height;

        int margin_left, margin_top;
        int div_w, div_h;
        int border_w;


        int old_div_w, old_div_h;

        int maxh;

        bool ruler_drawn;
        bool mapping_drawn;

        bool grid_drawn;

        int playhead;                                           /* where the playhead is for this canvas. only used for display. */

/*         /\* these are in logical units, not pixels *\/ */
/*         int w, h;                                               /\* viewport *\/ */
/*         int x, y;                                               /\* pan position *\/ */

        enum { PATTERN, SEQUENCE } mode;

        Grid *grid;                                             /* grid currently connected to this canvas */

        size_t size;
        cell_t **current, **previous;

        bool draw;                                              /* really drawing, or just checking size? */

        int rule;

        bool row_compact;                                      /* use row-compaction? */

        /* tables used for row-compaction */
        int rtn[128];                                           /* row-to-note */
        int ntr[128];                                           /* note-to-row */

        int shape;

        Viewport *vp;
        int w, h;

        uint p1, p2;                                             /* cursors */
    } m;

    int rtn ( int r ) const;
    int ntr ( int n ) const;

    void _update_row_mapping ( void );
    cell_t ** _alloc_array ( void );

    void redraw_ruler ( void );
    void redraw_mapping ( void );
    void draw_mapping ( void );
    void draw_ruler ( void );

    void _reset ( void );
    void _lr ( void );

    bool viewable_x ( int x );
    void draw_line ( int x, int flags );

public:

    enum { OFF, ON, TOGGLE };

    signal <void> signal_settings_change;
    signal <void> signal_draw;
    signal <void> signal_resize;

    Canvas ( );

    void handle_event_change ( void );
    void set ( int x, int y );
    void grid ( Grid *g );
    void changed_mapping ( void );
    Grid * grid ( void );
    void resize ( void );
    void resize_grid ( void );
    void resize ( int x, int y, int w, int h );
    void copy ( void );
    void clear ( void );
    void flip ( void );
    void draw_row_name ( int y, const char *name, int color );
    void draw_shape ( int x, int y, int shape, int state, int color, bool selected );
    void draw_dash ( int x, int y, int l, int shape, int color, bool selected );
    int draw_playhead ( void );
    void draw ( void );
    void redraw ( void );
    bool grid_pos ( int *x, int *y ) const;
    int is_row_name ( int x, int y );
    void unset ( int x, int y );
    void adj_color ( int x, int y, int n );
    void adj_length ( int x, int y, int n );
    void select ( int x, int y );
    void select_range ( void );
    void duplicate_range ( void );
    void crop ( void );
    void row_compact ( int n );
    void pan ( int dir, int n );
    void h_zoom ( float n );
    void v_zoom ( float n );
    void v_zoom_fit ( void );
    void notes ( char *s );
    char * notes ( void );
    void randomize_row ( int y );

    void delete_time ( void );
    void insert_time ( void );

    void move_selected ( int dir, int n );

};

inline int
Canvas::rtn ( int r ) const
{
    return m.row_compact ? m.rtn[ r ] : r;
}

inline int
Canvas::ntr ( int n ) const
{
    return m.row_compact ? m.ntr[ n ] : n;
}
