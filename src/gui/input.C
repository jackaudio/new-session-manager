
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

#include <cstring>

/* system */
#include <sys/types.h>
#include <unistd.h>

#include "../non.H"
#include "draw.H"
#include "../common.h"

#include "ui.H"

extern UI *ui;

void
async_exec ( const char *cmd )
{
    if ( fork() )
    {
        printf( "Executed command \"%s\"\n", cmd );
        return;
    }

    system( cmd );
    exit(0);
}

int
canvas_input_callback ( O_Canvas *widget, Canvas *c, int m )
{
    //  MESSAGE( "Hello, my name is %s", widget->parent()->label() );

    int ow, oh;

    int x, y;
    int processed = 1;

    x = Fl::event_x();
    y = Fl::event_y();


    ow = c->grid()->viewport.w;
    oh = c->grid()->viewport.h;

    switch ( m )
    {
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
                case FL_Delete:
                    if ( Fl::event_state() & FL_SHIFT )
                        c->grid()->clear();
                    else
                        c->grid()->delete_selected();
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
                        case '1':
                            c->h_zoom( 2.0f );
                            break;
                        case '2':
                            c->h_zoom( 0.5f );
                            break;
                        case '3':
                            c->v_zoom( 2.0f );
                            break;
                        case '4':
                            c->v_zoom( 0.5f );
                            break;
                        case ' ':
                            transport.toggle();
                            break;
                        case '[':
                        {
                            Grid *g = NULL;

#define IS_PATTERN (widget->parent() == ui->pattern_tab)
#define IS_PHRASE (widget->parent() == ui->phrase_tab)
#define IS_SEQUENCE (widget->parent() == ui->sequence_tab)

                            /* is there no nicer way to do this shit in c++? */
                            g = c->grid()->by_number( c->grid()->number() - 1 );

                            if ( g )
                            {
                                c->grid( g );
                                processed = 2;
                            }
                            break;
                        }
                        case ']':
                        {
                            Grid *g = NULL;

                            /* is there no nicer way to do this shit in c++? */
                            g = c->grid()->by_number( c->grid()->number() + 1 );

                            if ( g )
                            {
                                c->grid( g );
                                processed = 2;
                            }
                            break;
                        }
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
                        case 'c':
                        {
                            c->grid( c->grid()->create() );

                            ui->update_sequence_widgets();

                            break;
                        }
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

                        case 'm':
                            c->grid()->mode( c->grid()->mode() == MUTE ? PLAY : MUTE );
                            break;
                        case 's':
                            c->grid()->mode( c->grid()->mode() == SOLO ? PLAY : SOLO );
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
            switch ( Fl::event_button() )
            {
                case 1:
                    int note;
                    if ( ( note = c->is_row_name( x, y ) ) >= 0 )
                    {
                        DMESSAGE( "click on row %d", note );
                        Instrument *i = ((pattern *)c->grid())->mapping.instrument();

                        if ( i )
                        {
                            ui->edit_instrument_row( i, note );

                            c->changed_mapping();
                        }
                    }
                    else
                    {
                        if ( Fl::event_state() & FL_SHIFT )
                        {
                            c->start_cursor( x, y );
                            break;
                        }

                        if ( IS_PATTERN && Fl::event_state() & ( FL_ALT  | FL_CTRL ) )
                            c->randomize_row( y );
                        else
                            c->set( x, y );
                    }
                    break;
                case 3:
                    if ( Fl::event_state() & FL_SHIFT )
                    {
                        c->end_cursor( x, y );
                        break;
                    }

                    c->unset( x, y );
                    break;
                case 2:
                    c->select( x, y );
                    break;
                default:
                    processed = 0;
            }
            break;
        }
        case FL_RELEASE:
            break;
        case FL_DRAG:
            break;
/*         case FL_DRAG: */
/*         { */
/*             if ( ! lmb_down ) */
/*                 break; */

/*             //          c->grid()->move( x, y, nx ); */
/*             break; */
/*         } */
        case FL_MOUSEWHEEL:
        {
            if ( Fl::event_state() & FL_CTRL )
                c->adj_length( x, y, (0 - Fl::event_dy()) );
            else if ( Fl::event_state() & FL_ALT )
                c->adj_color( x, y, (0 - Fl::event_dy()) * 5 );
            else if ( Fl::event_state() & FL_SHIFT )
            {
                if ( Fl::event_dy() > 0 )
                {
                    c->pan( RIGHT, Fl::event_dy() * 5 );
                }
                else
                {
                    c->pan( LEFT, 0 - Fl::event_dy() * 5 );
                }
            }
            else
            {
                if ( Fl::event_dy() > 0 )
                {
                    c->pan( DOWN, Fl::event_dy() * 1 );
                }
                else
                {
                    c->pan( UP, (0 - Fl::event_dy()) * 1 );
                }
            }

            break;
        }
        default:
            processed = 0;
    }

    int nw, nh;
    nw = c->grid()->viewport.w;
    nh = c->grid()->viewport.h;

    // layout of canvas changed... requires clearing.
    if ( oh != nh || ow != nw )
        return 3;

    return processed;
}
