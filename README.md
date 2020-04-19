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
This repository contains no user interface. Recommendations for a good one might be added in the future.

## Fork and License
This is a fork of non-session-manager, by Jonathan Moore Liles <male@tuxfamily.net> http://non.tuxfamily.org/
under the GNU GENERAL PUBLIC LICENSE  Version 2, June 1991.

All files, except nsm.h kept in this repository are GPL "version 2 of the License, or (at your
option) any later version."

`nsm.h` is licenced under the ISCL.

Build files and FLTK definitions have no license header and will be removed.

The current repository builds the original `nsmd` and `jackpatch`.

## Build
Dependencies are jack2 and liblo, the OSC library.
The build system is meson.

```
meson build --prefix=/usr
cd build
ninja
sudo ninja install
```


