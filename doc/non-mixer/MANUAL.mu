
! title		Non Mixer User Manual
! author	Jonathan Moore Liles #(email,male@tuxfamily.org)
! date		January 21, 2010

-- Table Of Contents

: User Manual

:: The Mixer

/ Mixer
< non-mixer-complex.png

  The Non-Mixer is a stand-alone audio mixer, utilizing JACK as an
  audio subsystem. At the time of writing, the architecture of
  Non-Mixer is unique. By making the mixer stand-alone, concepts such
  as busses, sends, and inserts are eliminated, as the same goals can
  be achieved by simply adding more strips to the mixer.

  Start by creating a new project.

/ New Project
< new-project.png

  After the project has been created. Hit `a` or choose `Mixer/Add
  Strip` from the menu to add a new strip to the mixer.

::: Mixer Strip

/ Mixer Strip
< single-strip.png

  Each mixer strip has a name and color, each of which may be defined
  by the user. Names, but not colors, must be unique. In addition,
  each strip has controls to move it left or right (the arrows) in the
  display and to remove it entirely (the 'X').

  Strips start out in /narrow/ mode, with the /fader/ view
  enabled. Click the desired button to toggle the mode or view.

  The fader view comprises a large gain control and digital peak meter
  indicator. These are automatically connected to the default gain and
  meter modules of the strips signal chain.

  To see how an audio signal traveling through this strip will be
  processed, switch to its /signal/ view.

:::: Signal Processing


::::: Signal Chain

  The signal chain view of a mixer strip provides a way to view and
  manipulate the signal processing of a mixer strip.

:::::: Modules

/ Modules
< modules.png

  All signal processing in Non Mixer occurs in /Modules/. Modules are

  signal processing abstractions providing ports for audio and control
  I/O and, in addition, some simple user interface. Sink and source
  modules carry audio out of and into JACK.

  Modules are displayed as named blocks. Some modules may have
  additional GUI components.

  Each module has zero or more audio I/O ports and zero or more
  control ports. Audio routing between modules is handled
  automatically. Modules with mono audio configurations (one channel
  in, one channel out) can be automatically adjusted to support any
  number of discrete channels. Modules with more (related) channels,
  however, introduce restrictions on the order in which modules can be
  chained.

  An indicator in the upper left-hand corner of each module block
  indicates whether the module has any parameters bound to controls.

  Non Mixer has several built-in modules. They are:

= JACK
	= Performs JACK I\/O
= Gain
	= Applies gain in dB
= Meter
	= Digital Peak meter
= Mono Pan
	= Performs intensity panning of a mono signal into a stereo signal.
= Plugin
	= Hosts a LADSPA plugin

  Left-clicking on a module brings up the Module Parameter Editor.

  Shift+Left-clicking on a module brings up a menu which allows you to
  pick a new module to insert before this one in the chain.

  Control+Right-clicking on a module causes it to be removed from the
  chain (modules added by default cannot be removed).

::::::: Module Parameter Editor

/ Module Parameter Editor
< gverb-parameters-knobs.png

  The Module Parameter Editor is used to alter the values of a
  module's parameters, and, further more, to bind its parameters to
  controls. A menu button in the upper left-hand corner allows you to
  select between knob, vertical slider and horizontal slider controls.

/ Horizontal Sliders
< gverb-parameters-hsliders.png

/ Vertical Sliders
< gverb-parameters-vsliders.png

  Underneath each control is a bind button. Clicking adds a new
  control to the chain's /Controls/ view and binds it to the parameter
  in question. For simplicity, only one control at a time may be bound
  to a given parameter.

::::::: Controls

/ Control View
< controls.png

  The control view of a chain groups together all of the controls
  bound to parameters of modules in that chain. The default mode of
  controls is /Manual/. Right click on a control to bring up a menu
  which will allow you to select one of the available control I/O
  methods to use. When /Control Voltage/ (CV) is selected, a CV input
  port will be created on the containing mixer strip's JACK
  client. The control will now accept values from that input. A
  control bound and configured in this way can then be connected to
  the output of Non-DAW a control sequence using your favorite
  connection manager.

{ NOTE:
{ All knob and slider controls respond to mousewheel
{ events. Hold down the `Ctrl` key while scrolling the mousewheel to
{ achieve finer resolution.

:::: JACK I/O

  Each mixer strip is presented as a separate JACK "client". This
  helps to avoid the necessity of internally duplicating JACK's
  routing logic and, with JACK2, permits the possibility of parallel
  execution of mixer strip signal chains.

  The JACK client name of each strip will correspond to the name of the strip.

{ NOTE:
{ The JACK API makes implementing this far more difficult and kludgey than it should have to be.
{ Please petition your local JACK developer to accept jack_client_set_name() into the API.

/ Patchage
< non-mixer-and-non-daw-in-patchage.png
