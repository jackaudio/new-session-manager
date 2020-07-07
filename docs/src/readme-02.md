
## Documentation and Manual
See the generated website installed in your systems SHARE dir or docs/out/index.html from this
repository. Alternatively you can read the online version on our github page. All are the same
files. We also provide a set of manpages for each executable (see below).


## Fork and License
This is a fork of non-session-manager, by Jonathan Moore Liles <male@tuxfamily.net> http://non.tuxfamily.org/
which was released under the GNU GENERAL PUBLIC LICENSE  Version 2, June 1991.

All files, except nsm.h kept in this fork were GPL "version 2 of the License, or (at your
option) any later version."

`nsm.h` is licensed under the ISC License.

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
