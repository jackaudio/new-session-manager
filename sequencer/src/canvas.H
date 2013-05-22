
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
#include <FL/Fl_Group.H>

#include <sigc++/sigc++.h>
using namespace sigc;

class Mapping;

enum { LEFT, RIGHT, UP, DOWN, TO_PLAYHEAD, TO_NEXT_NOTE, TO_PREV_NOTE };

class Fl_Scrollbar;
class Fl_Slider;

class Canvas : public Fl_Group, public trackable
{
    class Canvas_Panzoomer;
    
    struct SelectionRect {
        int x1,y1,x2,y2;
            };

    SelectionRect _selection_rect;

    int _selection_mode;
    bool _move_mode;
    Canvas_Panzoomer *panzoomer;
    Fl_Slider *vzoom;

    /* these are grid coords, not pixels */
    int _old_scroll_x;
    int _old_scroll_y;

    struct {
        int origin_x, origin_y;
        int width, height;

        int margin_left, margin_top;
        int div_w, div_h;

        int old_div_w, old_div_h;

        int maxh;

        bool ruler_drawn;
        bool mapping_drawn;

        bool grid_drawn;

        int playhead;                                           /* where the playhead is for this canvas. only used for display. */

        enum { PATTERN, SEQUENCE } mode;

        Grid *grid;                                             /* grid currently connected to this canvas */

        size_t size;

        bool draw;                                              /* really drawing, or just checking size? */

        int rule;

        bool row_compact;                                       /* use row-compaction? */

        /* tables used for row-compaction */
        int rtn[128];                                           /* row-to-note */
        int ntr[128];                                           /* note-to-row */

        Viewport *vp;
        int w, h;

        uint p1, p2;                                            /* range cursors */
        uint p3, p4;                                            /* row cursors */
    } m;

    bool is_ruler_click ( void ) const;
    int rtn ( int r ) const;
    int ntr ( int n ) const;

    void _update_row_mapping ( void );

    void draw_mapping ( void );
    void draw_ruler ( void );

    void _reset ( void );
    void _lr ( void );

    bool viewable_x ( int x );

    void update_mapping ( void );

    static void cb_scroll (  Fl_Widget *w, void *v );
    void cb_scroll ( Fl_Widget *w );
    static void draw_clip ( void *v, int X, int Y, int W, int H );
    void draw_clip ( int X, int Y, int W, int H );

enum {
    SELECT_NONE = 0,
    SELECT_RECTANGLE,
    SELECT_RANGE
};

public:

    bool selection_mode ( void ) const { return _selection_mode; }
    void selection_mode ( bool b )
        {
            _selection_mode = b ? SELECT_RECTANGLE : SELECT_NONE;
        }

    enum { OFF, ON, TOGGLE };

    signal <void> signal_settings_change;

    Canvas ( int X, int Y, int W, int H, const char *L=0 );
    virtual ~Canvas ( );
    
    void cut ( void );
    void paste ( void );
    void redraw_playhead ( void );
    void handle_event_change ( void );
    void set ( int x, int y );
    void grid ( Grid *g );
    void changed_mapping ( void );
    Grid * grid ( void );
    void adj_size ( void );
    void resize_grid ( void );
    void resize ( int x, int y, int w, int h );
    void copy ( void );
    void draw_row_name ( int y, const char *name, int color );
    //    void draw_shape ( int x, int y, int w, int color );
    static void draw_dash ( tick_t x, int y, tick_t l, int color, int selected, void *userdata );
    void draw_dash ( tick_t x, int y, tick_t w, int color, int selected ) const;
    void damage_grid ( tick_t x, int y, tick_t w, int h );
    void draw_overlay ( void );
    void draw_playhead ( void );
    void draw ( void );
    bool grid_pos ( int *x, int *y ) const;
    int is_row_press ( void ) const;
    void unset ( int x, int y );
    void adj_color ( int x, int y, int n );
    void adj_length ( int x, int y, int n );
    void set_end ( int x, int y, int n );
    void select ( int x, int y );
    void select_range ( void );
    void invert_selection ( void );
    void duplicate_range ( void );
    void crop ( void );
    void row_compact ( int n );
    void pan ( int dir, int n );
    void can_scroll ( int *left, int *right, int *up, int *down );
    void h_zoom ( float n );
    void v_zoom ( float n );
    void v_zoom_fit ( void );
    void notes ( char *s );
    char * notes ( void );
    int playhead_moved ( void );

    void start_cursor ( int x, int y );
    void end_cursor ( int x, int y );

    void delete_time ( void );
    void insert_time ( void );

    void move_selected ( int dir, int n );
    void selected_velocity ( int v );

    virtual int handle ( int m );

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
