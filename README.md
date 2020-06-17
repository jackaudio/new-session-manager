# New Session Manager

## Introduction

New Session Manager (NSM) is a computer program to help handling other music production programs.
It is intended for Linux and other Free and Open Source operating systems.

You can create a session, or project, add programs to it and then use commands to save, start/stop,
hide/show all programs at once, or individually. At a later date you can then re-open the session
and continue where you left off.

All files belonging to the session will be saved in the same directory.

If you are a user (and not a programmer or packager) everything you need is to install NSM
through your distributions package manager and, highly recommended, Argodejo as a GUI (see below).

To learn NSM you don't need to know the background information from our documention, which
is aimed at developers that want to implement NSM support in their programs. Learn the GUI,
not the server and protocol.


## Bullet Points
* Drop-In replacement for the non-session-manager daemon nsmd and tools (e.g. jackpatch)
* Simple and hassle-free build system to make packaging easy
* Possibility to react to sensible bug fixes that would not have been integrated original nsmd
* Stay upwards and downwards compatible with original nsmd
* Conservative and hesistant in regards to new features and behaviour-changes, but possible in principle
* Keep the session-manager separate from the other NON* tools Mixer, Sequencer and Timeline.
* Protect nsmd from vanishing from the internet one day.
* The goal is to become the de-facto standard session manager for Linux distributions

## User Interface
It is highly recommended to use Argodejo ( https://www.laborejo.org/argodejo/ ) as graphical
user interface. In fact, if you install Argodejo in you distribution it will install NSM as
dependency and you don't need to do anything yourself with this software package.

This repository also contains the legacy FLTK interface simply called `new-session-manager`,
symlinked to `non-session-mnanager` for backwards compatibility. (e.g. autostart scripts etc.)

## Supported Clients

While NSM can start and stop any program it only becomes convenient if clients specifically
implement support. This enables saving and hiding the GUI, amongst other features.
Documentation and tutorials for software-developers will be added at a later date.

## Fork and License
This is a fork of non-session-manager, by Jonathan Moore Liles <male@tuxfamily.net> http://non.tuxfamily.org/
which was released the GNU GENERAL PUBLIC LICENSE  Version 2, June 1991.

All files, except nsm.h kept in this fork were GPL "version 2 of the License, or (at your
option) any later version."

`nsm.h` is licenced under the ISCL.

New-Session-Manager changed the license to GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007.
See file COPYING

## Build
The build system is meson.

This repository builds `nsmd` and `jackpatch`. Dependencies are jack2 and liblo, the OSC library.
If your system has FLTK installed (detected by the first step below) meson will enable building
of `nsm-proxy` and legacy GUI `new-session-manager` as well.

```
meson build --prefix=/usr
cd build && ninja
sudo ninja install
```

## Names of Executable Files and Symlinks

Some distributions (and possibly local laws) prevent a forked software project from creating
executable files under the same name, if the name itself is an original work subject to copyright,
which it arguably is for the "NON-"-suite. Therefore New Session Manager renamed
`non-session-manager` to `new-session-manager`. Installing will also create a symlink to
`non-session-mnanager` for backwards compatibility. (e.g. autostart scripts etc.).
