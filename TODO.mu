
! title     TODO list for the Non-sequencer

--

; General

* TODO: {3} fix transport seeking to the middle of a phrase/pattern in Sequence Mode.
* TODO: {3} deal with dropped ticks from timebase master better when running as slave.
* TODO: {2} move keybindings into menus for discoverability (like in Non-DAW)
* TODO: {2} improve or get rid of the transport.valid test at initialization time.
* TODO: {2} allow deletion and renumbering of individual patterns and phrases.
* TODO: {2} add option to create new instrument defintion.
* TODO: {1} per phrase tempo setting? Perhaps a percentage of global tempo?
* TODO: {1} new Phrase playback mode. In this mode, a single phrase is looped, and the next phrase to be looped can be queued. This is similar the the playlist, but not linear from bar 1.
* TODO: {1} live performance record. Does this have to be internal to Non? Is there no jack MIDI capable recorder that could be connected to all non ports? How would ports be handled anyway? As separate tracks in an SMF-1 file, perhaps?
* TODO: {1} guess key signature of imports?
* TODO: {1} figure out how to handle SysEx events with Jack MIDI (packetize them?)
* TODO: {1} add uneditable "other" tab to event list widget.
* TODO: {1} add mode for disconnected operation. This is especially important for the situation where there is a timebase master when Non is  started that exits before Non is closed, resulting in a lack of BBT information on the transport and a subsequent crash.
* TODO: {1} add global setting for composer name/copyright to be included in song files.
* TODO: {1} add chords to scale list... a pattern using the scale of a chord would contain a melody within that chord.
* TODO: {1} add channel field to event list widget (but channel bits in pattern event lists are currently meaningless.)
* TODO: {1} add 'compaction' action to remove unnamed phrases/patterns and ajdust their numbers approrpiately (offline)?

; Canvas

* TODO: {3} update phrase height when number of patterns changes... (good use for a signal?)
* TODO: {2} update beat/measure line drawing when BPB changes.
* TODO: {2} split canvas into separate widgets (ruler, names, canvas)?
* TODO: {1} phrases need a way to show/hide relevant patterns. The whole "mapping" system is a total mess. How about only showing rows containing events? How would one add a pattern in this system? A button that adds an event for the given pattern?
* TODO: {1} custom scrollbar widget (dots)
* TODO: {1} add vertical scrollbar widget to canvas.
