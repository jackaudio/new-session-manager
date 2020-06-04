# New Session Manager

## Mission Statement
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

## Names of Executable Files and Symlinks Some distributions (and possibly local laws) prevent a
forked software project from creating executable files under the same name, if the name itself is
an original work subject to copyright, which it arguably is for the "NON-"-suite. Therefore New
Session Manager renamed `non-session-manager` to `new-session-manager`. Install will also create a
symlink to `non-session-mnanager` for backwards compatibility. (e.g. autostart scripts etc.).
