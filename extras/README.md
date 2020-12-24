# New Session Manager - Extras

Each subdirectory in /extras holds additional libraries, software, documentation etc.

They are included for convenience, e.g. the library pynsm is useful for client-programmers,
and also developed by the same author as New-Session-Manager.

Each "extra" is standalone regarding license and build process. They are not build or installed
through nsm(d) meson build. The main programs in this repository do not depend on files in /extra in
any way. From a technical point of view `/extras` could be safely deleted.
