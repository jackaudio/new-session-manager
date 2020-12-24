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
through your distributions package manager and, highly recommended, Agordejo as a GUI (see below).

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
* The goal is to become the de-facto standard music session manager for Linux distributions

## User Interface
It is highly recommended to use Agordejo ( https://www.laborejo.org/agordejo/ ) as graphical
user interface. In fact, if you install Agordejo in you distribution it will install NSM as
dependency and you don't need to do anything yourself with this software package.

This repository also contains the legacy FLTK interface simply called `nsm-legacy-gui`,
symlinked to `non-session-manager` for backwards compatibility. (e.g. autostart scripts etc.)

## Supported Clients

While NSM can start and stop any program it only becomes convenient if clients specifically
implement support. This enables saving and hiding the GUI, amongst other features.
Documentation and tutorials for software-developers will be added at a later date.

## Documentation and Manual

Our documentation contains the API specification for the NSM protocol, which is the central document
if you want to add NSM support to your own application.

You can find html documentation installed to your systems SHARE dir or docs/out/index.html in this
repository.
There is also an online version https://linuxaudio.github.io/new-session-manager/

We also provide a set of manpages for each executable (see Build).


## Fork and License
This is a fork of non-session-manager, by Jonathan Moore Liles <male@tuxfamily.net> http://non.tuxfamily.org/
which was released under the GNU GENERAL PUBLIC LICENSE  Version 2, June 1991.

All files, except nsm.h kept in this fork were GPL "version 2 of the License, or (at your
option) any later version."

`extras/nsm.h/nsm.h` is licensed under the ISC License.

New-Session-Manager changed the license to GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007.
See file COPYING

Documentation in docs/ is licensed Creative Commons CC-By-Sa.
It consist of mostly generated files and snippet files which do not have a license header for
technical reasons.
All original documentation source files are CC-By-Sa Version v4.0 (see file docs/src/LICENSE),
the source file docs/src/api/index.adoc is a derived work from NON-Session-Managers API file which
is licensed CC-By-Sa v2.5. Therefore our derived API document is also CC-By-Sa v2.5
(see files docs/src/api/readme.txt and docs/src/api/LICENSE)


## Build
The build system is meson.

This is a software package that will compile and install multiple executables:
* `nsmd`, the daemon or server itself. It is mandatory.
  * It has no GUI.
  * Dependency is `liblo`, the OSC library.
* `jackpatch`, NSM client to save and remember JACK connections.
  * It has no GUI.
  * Dependencies are `JACK Audio Connection Kit` and `liblo`, the OSC library.
  * Can be deactivated (see below) `-Djackpatch=false`
* `nsm-legacy-gui`, Legacy GUI for the user
  * Formerly known as "non-session-manager"
  * Dependencies are `FLTK`>=v1.3.0 and `liblo`, the OSC library.
  * Can be deactivated (see below) `-Dlegacy-gui=false`
* `nsm-proxy`, NSM GUI Client to run any program without direct NSM support
  * Dependencies are `FLTK`>=v1.3.0, `fluid` (FLTK Editor/compiler, maybe in the same package as FLTK, maybe not) and `liblo`, the OSC library.
  * Can be deactivated (see below) `-Dnsm-proxy=false`


```
meson build --prefix=/usr
#or disable individual build targets:
#meson build --prefix=/usr -Dlegacy-gui=false -Dnsm-proxy=false -Djackpatch=false
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
`non-session-manager` to `nsm-legacy-gui`. Installing will also create a symlink to
`non-session-manager` for backwards compatibility. (e.g. autostart scripts etc.).
