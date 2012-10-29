
#include <jack/jack.h>
#include <MIDI/midievent.H>
#include "common.h"

enum { CONTROL, PERFORMANCE };


bool midi_input_event ( int port, MIDI::midievent *e );
bool midi_is_active ( void );
MIDI::midievent * midi_input_event ( int port );
void midi_output_event ( int port, const MIDI::midievent *e );
void midi_output_event ( int port, const MIDI::midievent *e, tick_t duration );
void midi_all_sound_off ( void );
const char * midi_init ( const char *name );
void midi_shutdown ( void );
void midi_output_immediate_event ( int port, const MIDI::midievent *e );
