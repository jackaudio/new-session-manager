
! title		The Non DAW
! author	Jonathan Moore Liles #(email,male@tuxfamily.org)
! date		March 1, 2008
! extra		#(image,logo,logo.png)

-- Table Of Contents

: Overview

:: Description

  The Non DAW is a powerful, reliable and fast modular Digital Audio
  Workstation system, released under the GNU General Public License
  (GPL). It utilizes the JACK Audio Connection Kit for
  inter-application audio I\/O and the FLTK GUI toolkit for a fast and
  lightweight user interface.

  Please see the #(url,MANUAL.html,Manual) for more information.

:: What it is not

  Non-DAW is *not* a wave editor. It is not a beat slicer. It is not a
  granular synthesis engine. It is *not* a clone of some proprietary DAW.  It
  is not an /insert name of proprietary audio thing here/ killer. It is *not*
  limiting and restricting.  It is *not* a monolithic DAW with internal mixing
  or EQ DSP. Non-DAW is intended to be one tool among many in your Linux audio
  toolbox.

:: What is a DAW?

  The acronym DAW stands for Digital Audio Workstation. Of course, Non is
  software, so when we say DAW we imply a purely software based system.  A DAW
  is used by modern studio engineers to record and arrange multitrack sessions
  of different musicians into a single song. Perhaps a more noble use of a
  DAW, and the one for which Non-DAW was specifically written, is to provide
  the mutli-instrumentalist individual with all the software necessary to
  quickly and conveniently record and arrange his compositions and produce a
  professional quality result.

  In this author's opinion, a DAW comprises the following functionality:

* Non-linear, non-destructive arrangement of portions of audio clips.
* Tempo and time signature mapping, with editing operations being closely aligned to this map.

  Since Non uses JACK for IO, some things traditionally considered to be within
  the scope of a monolithic DAW can be pared out into JACK and Non Mixer:

* Signal routing
* Audio mixing
* Hosting of plugins

:: Why write another one?

  First and foremost, we can disregard all non-free DAWs because we do not
  waste our precious time and spirit on non-free and\/or proprietary software.
  This excludes virtually every other DAW in existence. Secondly, we require a
  DAW that runs on the GNU\/Linux operating system in conjunction with other
  free software, such as the JACK Audio Connection Kit, in a modular and
  cooperative and manner.  Finally, we require a program that is powerful,
  fast, and reliable.  No other software meets these requirements.

  The design of the Non DAW differs substantially from others. This is a good
  thing; for a clone of a bad design is doomed from the start.

  There is only one other DAW that is capable and free software, and its name
  is Ardour.  Suffice it to say that the architecture of Ardour is incompatible
  with the requirements of speed and reliability. Other DAW-like free software
  programs, including Traverso and QTractor, are similarly limited (being of
  similar design), but suffer the additional burden of cumbersome legacy ALSA
  support and very a limited feature set.

  Given these options, we had no choice but to start from scratch, this time on
  a solid foundation, rather than attempting (in vain) to shoehorn good design
  into an existing code base.

:: Features

  Non-DAW shares many features in common with other, similar projects. However,
  Non-DAW's unique architecture permits surprising new functionality.

::: Journaled Projects

  Unlike legacy DAWs, which keep project state in huge, memory wasting, hard
  to manage XML (or binary equivalent) trees, Non-DAW has the unique ability
  to store project state in a compact continuous journal of bidirectional
  delta messages--similar to the journal part of journaling filesystems--in
  plain ASCII.

  The Non-DAW disk format takes the form of a journal of delta messages. Each
  project file contains the complete history of that project since the last
  (optional) compaction operation.  These journals are so terse that it is
  practical to keep the complete history of a project from the time it was
  first opened. No XML or other bloated, buggy, resource hungry format is
  employed. (Anyone suggesting the use of XML for anything related to this
  project will be shot on sight with incendiary rounds.)

  This has a number of highly desirable consequences. Among them:

+ Zero time spent 'saving' projects.
+ No need to 'save' projects manualy.
+ No need for CPU and RAM wasting 'autosave' function.
+ In the (unlikely) event of a crash, at most *one* transaction (user action) may be lost, and the project will *not* be invalidated.
+ Unlimited undo--potentially going back to the very moment the project was created (state of the template it was based on).
+ Undo history requires no additional RAM.
+ Project format is insanely simple and easy to manipulate with sed or awk scripts, should the need arise (see the included `remove-unused-sources` script for an example).

  Non-DAW's journalling capability can drastically change your workflow. No
  longer will you fear a system failure. No longer will your pinky finger
  become sore from hitting Control-S after every important change. No longer
  will you have to attempt, in vain, to manually edit a completely
  incomprehensible XML 'document', because Ardour has corrupted its memory and
  therefore the project you 'saved'.

::: Non-destructive editing

  Sound sources (audio files) are represented by /regions/. Any number of
  regions may represent different parts of the same source. All editing is
  performed on these region structures--the sound sources themselves are
  considered read-only (except for captures in-progress).

::: Unlimited tracks

  Tracks in a DAW are unlike tracks on tape in that a single track can
  contain more than one channel of audio. Each audio track has its own
  record, mute, solo, and gain, as well an active take and any number
  of inactive takes. A track may also have any number of annotation
  and control sequences associated with it.

::: Unlimited takes

  A /take/ is a sequence of regions. Each track has /current take/, implied by
  'the track', as well as any number of other, inactive takes. A track may be
  set to display all takes simultaneously, to ease the process of reviewing
  past takes or stitching together a new take from parts of previous takes.
  Old takes may be deleted, either one by one or all at once, when they are no
  longer required. Takes may not be transferred between tracks (there's no
  technical reason why they can't, but allowing this would be bad design).

::: Cross-fades

  Where regions overlap, a cross-fade exists. This means that the transition
  from region A to region B will be gradual rather than abrupt. The shape of
  the gain curve may be selected separately for region A and B of the
  cross-fade. Available curves include: Linear, Sigmoid, Logarithmic, and
  Parabolic.

::: Automation

  Each track can have associated with it any number of /control sequences/, a
  subset of which may be visible at any one time. Each control sequence
  comprises a series of /control points/, which collectively represent a graph
  of changes to a single controllable value over time. Anything may be
  controlled by a control track, including external software supporting OSC or
  MIDI control, although the most common application is mixer gain automation,
  where the value controlled is the fader level in the mixer.

::: Time/tempo mapping

  The time and tempo maps (rulers) affect where and how many bar\/beat lines
  are drawn. During playback they affect the time\/tempo of the JACK transport
  so that other programs, like the Non-Sequencer, can follow along in sync.

:: Components

  The Mixer and the Timeline are separate programs, connected through JACK.

::: Timeline

  All operations on the timeline are journaled, and therefore reversible.

  The following data belong to the timeline:

= Tracks and Takes
	= Each Track has a number of input and output ports, a name, and
	= any number of attached sequences. All sequences but the current
	= (topmost) are inactive and do not generate sound or accept
	= captures. These sequences are referred to as /Takes/. Previous
	= takes may be swapped with the current sequence and all takes
	= may be shown on screen at once for easy splicing. Each track
	= can also have any number of Control Sequences attached to it,
	= in which case all control sequences generate control output
	= unless disconnected. The height of a track may be adjusted
	= and a track can be muted, soloed, or record-enabled.
= Regions
	= Regions are the most common object on the timeline.  Each region
	= represents a segment of some particular audio file. Waveforms
	= of all regions belonging to the same source are displayed in
	= the same hue. Each region has a normalization value and regions
	= can be selected individually or operated on in groups. Each
	= region has a fade-in and fade-out curve, and when two regions
	= overlap, this constitutes a cross-fade.
= Control Points
	= Control points are arbitrarily placed points on a curve (or
	= line) from which continuous control values are interpolated
	= and sent out a JACK port (like a control voltage).
= Time and Tempo Points
	= Time and Tempo points control the tempo and meter throughout
	= time.  This information is used for drawing the measure lines
	= and snapping to the grid, as well as informing other JACK
	= clients of tempo changes throughout a song.
= Annotation Points
	= Cue points are textual markers on the timeline. Common names
	= for cue points include "Verse 1", "Bridge", etc.
= Annotation Regions
	= Annotation Regions are annotations with a definite duration.
	= These are useful for representing lyrics or other notes of a
	= timely nature. Each track may have any number of annotation
	= sequences associated with it, and these sequences can contain
	= a free mix of annotation points and annotation regions.

; What does freedom have to do with this software?

  Non is /free software/. This means, briefly, that you are free use it as
  *you* wish, free to examine and adapt the source code, free to share it with
  your friends, and free to publish your changes to the source code.
  Furthermore, Non is /copyleft/, which means that you are free from the
  threat of some other entity taking over and denying you the above freedoms.
  The /free/ part of /free software/ doesn't refer to price any more than the
  /free/ in /free speech/ does.

  To learn why free software is so important to us (and why it should be
  important to you), please see the Free Software Foundation's website:

  #(url,http:\/\/www.fsf.org\/licensing\/essays\/free-sw.html,What is Free Software?)
  #(url,http:\/\/www.fsf.org\/licensing\/essays\/copyleft.html,What is Copyleft?)

; Donations

  Donations can take many forms. You can donate your time in code, either by
  sending it to me for review or cloning the git repository and publishing one
  containing your changes. You can donate your time in testing, documentation,
  artwork, indexing, etc. Or, if you don't feel that you possess the time or
  skills required for the above forms of donation, you can donate money
  instead. Money donated will help to ensure that I have the free time, good
  nutrition and enthusiasm required to implement new features.  It can also be
  a more palpable way of saying "Thanks for caring." or "Job well done!"

  If you don't love this software, don't feel guilty about not contributing.
  If you do love it, then please help me improve it--in whatever manner you
  think is appropriate.

  #(url,http:\/\/non.tuxfamily.org\/donation.html,Make a donation)

; Distribution

  Development of the Non-DAW and Non-Mixer can be followed with Git:

> git clone git://git.tuxfamily.org/gitroot/non/daw.git

  There are no pre-compiled binaries available.

; Requirements

  The following libraries are required to build Non DAW and Non Mixer

* FLTK >= 1.1.7 (with `fluid`)
* JACK >= 0.103.0
* libsndfile >= 0.18.0

; Community

  Feel free to drop by the `#non` channel on irc.freenode.net.

  There is a mailing list `non-daw@lists.tuxfamily.org`.

  To subscribe, send a message with the subject 'subscribe' to
  #(email,non-daw-request@lists.tuxfamily.org).

  You can also browse the #(url,http:\/\/listengine.tuxfamily.org\/lists.tuxfamily.org\/non-daw\/,archive).
