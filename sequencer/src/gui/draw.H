
#pragma once

/* canvas node states */
enum {
/* real */
    EMPTY,                                                      /* nothing */
    FULL,                                                       /* dot or dash head */
    PARTIAL,
    CONTINUED,                                                  /* dash tail */
    SELECTED,
/* virtual */
    HIT,                                                        /* playhead hit */
    LINE,                                                       /* beat line */
    PLAYHEAD,
    MAX_STATE,
};

#define MAX_REAL_STATE HIT

#define STATE_MASK 0x0F
#define STATE_FLAG_MASK (~ (STATE_MASK) )

/* flags */
enum {
    F_PLAYHEAD  = 1 << 0,                                     /* playhead is on item */
    F_P1        = 1 << 1,
    F_P2        = 1 << 2,
    F_SELECTION = 1 << 3                                      /* item is part of the selection box */
};


void init_colors ( void );
void gui_status ( const char *fmt, ... );
