
#include <jack/jack.h>

#include "common.h"

enum { CONTROL, PERFORMANCE };

class midievent;

midievent * midi_input_event ( int port );
void midi_output_event ( int port, const midievent *e );
void midi_output_event ( int port, const midievent *e, tick_t duration );
void midi_all_sound_off ( void );
const char * midi_init ( void );
void midi_shutdown ( void );
void midi_output_immediate_event ( int port, const midievent *e );
