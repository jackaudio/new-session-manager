
! title		The Non Sequencer
! author	Jonathan Moore Liles #(email,wantingwaiting@users.sf.net)
! extra		#(url,http://non.tuxfamily.org,Home) #(image,logo,logo.png)

--

; Description

  The Non Sequencer is a powerful real-time, pattern-based MIDI sequencer for
  Linux--released under the GPL.  Filling the void left by countless DAWs,
  piano-roll editors, and other purely performance based solutions, it is a
  compositional tool--one that transforms MIDI music-making on Linux from a
  complex nightmare into a pleasurable, efficient, and streamlined process.

  Please see the #(url,MANUAL.html,manual) for more information.

; What it is not

  Non is *not* a plain MIDI recorder, tracker, software synthesizer, notation
  editor or AI system. There are other programs available to do those things.
  Non is intended to be one tool among many in your Linux audio toolbox.

  Everything in Non happens /on-line/, in realtime. Music is composed live,
  while the transport is running.

; Distribution

  Development of the Non Sequencer can be followed with Git:

> git clone git://git.tuxfamily.org/gitroot/non/sequencer.git

#  or
#
# > git clone git://repo.or.gz/src/git/non.git

  There are no pre-compiled binaries available.

; Requirements

  The following libraries are required to build Non.

* FLTK 1.1.x
* JACK >= 0.103.0
* sigc++ 2.0

  The following are optional:

* LASH >= 0.5.4
