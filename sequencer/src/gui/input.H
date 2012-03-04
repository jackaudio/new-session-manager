
#pragma once

#include "../canvas.H"
#include "../common.h"

class O_Canvas;

void disp_message ( char *s );
void async_exec ( const char *cmd );
int canvas_input_callback ( O_Canvas *widget, Canvas *c, int m );
int disp_init ( int argc, char **argv );
