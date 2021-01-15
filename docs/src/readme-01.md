
## Introduction

New Session Manager (NSM) is a tool to assist music production by grouping standalone programs into sessions.
Your workflow becomes easy to manage, robust and fast by leveraging the full potential of cooperative applications.

NSM is free in every sense of the word free of cost, free to share and use, free of spyware or ads, free-and-open-source.

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
