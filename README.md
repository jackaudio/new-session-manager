# New Session Manager

## Introduction

New Session Manager (NSM) is a tool to assist music production by grouping standalone programs into sessions.
Your workflow becomes easy to manage, robust and fast by leveraging the full potential of cooperative applications.

It is a community version of the "NON Session Manager" and free in every sense of the word:
free of cost, free to share and use, free of spyware or ads, free-and-open-source.

You can create a session, or project, add programs to it and then use commands to save, start/stop,
hide/show all programs at once, or individually. At a later date you can then re-open the session
and continue where you left off.

All files belonging to the session will be saved in the same directory.

If you are a user (and not a programmer or packager) everything you need is to install NSM
through your distributions package manager and, highly recommended, Argodejo as a GUI (see below).

To learn NSM you don't need to know the background information from our documentation, which
is aimed at developers that want to implement NSM support in their programs. Learn the GUI,
not the server and protocol.


## Bullet Points
* Drop-In replacement for the non-session-manager daemon nsmd and tools (e.g. jackpatch)
* Simple and hassle-free build system to make packaging easy
* Possibility to react to sensible bug fixes that would not have been integrated original nsmd
* Stay upwards and downwards compatible with original nsmd
* Conservative and hesitant in regards to new features and behaviour-changes, but possible in principle
* Keep the session-manager separate from the other NON* tools Mixer, Sequencer and Timeline.
* Protect nsmd from vanishing from the internet one day.
* The goal is to become the de-facto standard session manager for Linux distributions

## User Interface
It is highly recommended to use Argodejo ( https://www.laborejo.org/argodejo/ ) as graphical
user interface. In fact, if you install Argodejo in you distribution it will install NSM as
dependency and you don't need to do anything yourself with this software package.

This repository also contains the legacy FLTK interface simply called `new-session-manager`,
symlinked to `non-session-manager` for backwards compatibility. (e.g. autostart scripts etc.)

## Supported Clients

While NSM can start and stop any program it only becomes convenient if clients specifically
implement support. This enables saving and hiding the GUI, amongst other features.
Documentation and tutorials for software-developers will be added at a later date.

## Fork and License
This is a fork of non-session-manager, by Jonathan Moore Liles <male@tuxfamily.net> http://non.tuxfamily.org/
which was released under the GNU GENERAL PUBLIC LICENSE  Version 2, June 1991.

All files, except nsm.h kept in this fork were GPL "version 2 of the License, or (at your
option) any later version."

`nsm.h` is licensed under the ISCL.

New-Session-Manager changed the license to GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007.
See file COPYING

## Build
The build system is meson.

This is a software package that will compile and install multiple executables:
* `nsmd`, the daemon or server itself. It is mandatory.
  * It has no GUI.
  * Dependencies are `JACK Audio Connection Kit` and liblo`, the OSC library.
* `jackpatch`, NSM client to save and remember JACK connections.
  * It has no GUI.
  * Dependencies are `JACK Audio Connection Kit` and `liblo`, the OSC library.
  * Can be deactivated (see below) `-Djackpatch=false`
* `new-session-manager`, Legacy GUI for the user
  * Dependencies are `FLTK` and `liblo`, the OSC library.
  * Can be deactivated (see below) `-Dnew-session-manager=false`
* `nsm-proxy`, NSM GUI Client to run any program without direct NSM support
  * Dependencies are `FLTK`, `fluid` (FLTK Editor/compiler, maybe in the same package as FLTK, maybe not) and `liblo`, the OSC library.
  * Can be deactivated (see below) `-Dnsm-proxy=false`


```
meson build --prefix=/usr
#or disable individual build targets:
#meson build --prefix=/usr -Dnew-session-manager=false -Dnsm-proxy=false -Djackpatch=false
cd build && ninja
sudo ninja install
```

Optionally you can skip `sudo ninja install` and run all executables from the build-dir.
In this case you need to add the build-dir to your PATH environment variable so that the tools
can find each other.

## Names of Executable Files and Symlinks

Some distributions (and possibly local laws) prevent a forked software project from creating
executable files under the same name, if the name itself is an original work subject to copyright,
which it arguably is for the "NON-"-suite. Therefore New Session Manager renamed
`non-session-manager` to `new-session-manager`. Installing will also create a symlink to
`non-session-manager` for backwards compatibility. (e.g. autostart scripts etc.).
