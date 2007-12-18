
#pragma once

/* enum { */
/*   EMPTY, */
/*   FULL, */
/*   PARTIAL, */
/*   CONTINUED, */
/*   HIT, */
/*   RING, */
/*   SELECTED, */
/*   LINE, */
/*   PLAYHEAD */
/* }; */

/* canvas node states */
enum {
/* real */
    EMPTY,                                                      /* nothing */
    FULL,                                                       /* dot or dash head */
    PARTIAL,
    CONTINUED,                                                  /* dash tail */
/* virtual */
    HIT,                                                        /* playhead hit */
    LINE,                                                       /* beat line */
    PLAYHEAD,
    SELECTED,
    MAX_STATE,
};

#define MAX_REAL_STATE HIT

#define STATE_MASK 0x0F
#define STATE_FLAG_MASK (~ (STATE_MASK) )

/* flags */
enum {
    F_PLAYHEAD = 1 << 0,                                     /* playhead is on item */
    F_P1       = 1 << 1,
    F_P2       = 1 << 2,
    F_SELECTED = 1 << 3                                      /* item is selected */
};


/* shapes */
enum {
    CIRCLE,
    HALF_CIRCLE,
    SQUARE,
    DIAMOND,
    HEXAGON
};

const int ruler_height = 14;

void init_colors ( void );
int gui_draw_ruler ( int x, int y, int w, int div_w, int div, int ofs, int p1, int p2 );
int gui_draw_string ( int x, int y, int w, int h, int color, const char *s, bool draw );
void gui_draw_shape ( int x, int y, int w, int h, int bw, int shape, int state, int flags, int color );
void gui_clear_area ( int x, int y, int w, int h );
void gui_status ( const char *fmt, ... );
