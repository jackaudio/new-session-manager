
! title		The Non Mixer
! author	Jonathan Moore Liles #(email,male@tuxfamily.org)
! date		February 1, 2010
! extra		#(image,icon,icon.png)

-- Table Of Contents

: Overview

< non-mixer-complex.png

:: Description

  The Non Mixer is a powerful, reliable and fast modular Digital Audio
  Mixer, released under the GNU General Public License (GPL). It
  utilizes the JACK Audio Connection Kit for inter-application audio
  I\/O and the FLTK GUI toolkit for a fast and lightweight user
  interface.

  Please see the #(url,MANUAL.html,Manual) for more information (and
  lots of screenshots).

:: Why write another one?

  At the time work began on Non Mixer, there was no other powerful,
  fast, and light stand-alone free-software JACK mixer available.
  There was (and is) LiveMix, however LiveMix is neither fast nor
  light, and it wasn't able to accept the necessary external control
  data. SpiralSynthModular, strangely enough, was the closest thing
  the author could find to the tool he needed (it has gain\/mix
  modules and all modules accept Control Voltage input). SSM is truly
  an excellent, if neglected, program, but the modules-on-a-canvas
  model with manual routing is far too cumbersome of an arrangement
  for real world mixing tasks. Instead of creating another monolithic
  DAW with built-in routing and mixing, the author choose to follow
  the time tested Unix design philosophy of one tool per task. The
  most obvious point of division is between the timeline and the
  mixer. Drawing the line here allows routing and mixing to actually
  occur within JACK, which is an infinitely more flexible arrangement
  than the alternative.

:: Features

  Non-Mixer shares many features in common with other, similar
  projects. However, Non-Mixer's unique architecture permits
  surprising new functionality.

::: Stand-alone Implementation

  The Non Mixer is a stand-alone application. It is a complement to Non
  DAW, but neither program requires the other in order to function.

  Implementing the mixer functionality in a separate program, connected
  to Non-DAW via JACK presents a number of advantages:

* Eliminates the need for an internal connection management interface
* Improves overall system stability
* Increases parallelism (utilizes multiple cores)
* Adds flexibility
* Eliminates the need for cumbersome concepts and interfaces such as 'sends', 'groups', 'inserts' and 'busses'.

  Multiple instances of the mixer can be run together in order to
  organize groups of channels and manage them with your preferred
  window manager.

  Each mixer strip runs as a separate JACK client. In JACK2, this can
  translates into the DSP workload being spread across multiple CPU
  cores.

  Since the Mixer is an entirely separate unit, you may use any JACK
  capable mixer client you want instead of, or in myriad combination
  with, the Non-DAW Mixer. The one called LiveMix works well, but
  lacks many features of Non's mixer.  All operations on the mixer
  affect the current mix set state only and are not journaled.

  The mixer's design is modular, with all DSP occurring in discrete
  modules. One module hosts LADSPA plugins and the mixer is capable of
  receiving control (automation) data for any module parameter from
  Non-DAW (or another program) via JACK.

  Control data is expressed either as Control Voltage (CV) or Control
  Signals.

::: Modular Signal Processing

  All signal processing in Non Mixer occurs in /Modules/. This
  includes input and output from\/to JACK. The modular design helps to
  restrain the total program complexity, and this in turn increases
  flexibility and reliability. Even the built-in gain stage and meter
  are modules (and you can place as many meters as you like, wherever
  you like, in the signal chain, eliminating the post\/pre concept).

::: Parallel Processing

  Because each mixer strip in Non Mixer runs as a separate JACK client
  (in its own thread), JACK has the opportunity, upon analysis of the
  connection graph, to execute some or all strips in parallel. This is
  a highly desirable behavior for those having multi-CPU and\/or
  multi-core configurations.

::: Multiple Instances

  Because Non Mixer is stand-alone, it is possible to run multiple
  instances. One reason you might want to do this is so that you can
  group sets of strips out of a large total number of strips. For
  example, you might want to keep all of your drum strips together. By
  using multiple Non Mixer instances, you are able to manage these
  groups of strips with the familiar navigation facilities of your
  chosen window manager. Once again, a modular design allows us to
  maintain a good balance of complexity across user interface levels
  without duplicating and internalizing the functionality already
  available at a higher level.

::: LADSPA Plugins

  LADSPA plugins are hosted by the /Plugin/ module and can be inserted
  anywhere in the signal chain (contingent on compatibility of the
  I\/O configuration at that point).

::: Unlimited Strips

  There is no limit imposed by Non Mixer on the total number of strips
  or Mixer instances you can create.

::: Automation

  Any module parameter may be bound to a /control/. The control may be
  controlled via the GUI, or externally via a Control Voltage signal,
  such as is output by a Non-DAW control sequence.
  
  All module parameters are alterable via OSC messages, regardless of
  whether or not they have /controls/ defined.

::: Spatialization

  Plugins supporting Ambisonics panning are automatically assigned a
  special control called a Spatializer. This allows for easy and
  positioning of sound sources.

; What does freedom have to do with this software?

  Non is /free software/. This means, briefly, that you are free use
  it as *you* wish, free to examine and adapt the source code, free to
  share it with your friends, and free to publish your changes to the
  source code.  Furthermore, Non is /copyleft/, which means that you
  are free from the threat of some other entity taking over and
  denying you the above freedoms.  The /free/ part of /free software/
  doesn't refer to price any more than the /free/ in /free speech/
  does.

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

  Development of the Non-Mixer can be followed with Git:

> git clone git://git.tuxfamily.org/gitroot/non/non.git

  There are no pre-compiled binaries available.

; Requirements

  The following libraries are required to build Non DAW and Non Mixer

* JACK >= 0.103.0
* liblrdf >= 0.1.0
* liblo >= 0.26

; Community

  Feel free to drop by the `#non` channel on irc.freenode.net.

  There is a mailing list `non-mixer@lists.tuxfamily.org`.

  To subscribe, send a message with the subject 'subscribe' to
  #(email,non-mixer-request@lists.tuxfamily.org).

  You can also browse the #(url,http:\/\/listengine.tuxfamily.org\/lists.tuxfamily.org\/non-mixer\/,archive).
