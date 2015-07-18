SDLPAL
======

SDLPAL is an SDL-based reimplementation of the classic Chinese-language RPG
"Xian Jian Qi Xia Zhuan" (also known as PAL or Legend of Sword and Fairy).


LICENSE
=======

SDLPAL is Copyrighted (c) 2009-2015 Wei Mingzhi <whistler_wmz@users.sf.net>.
All rights reserved.

SDLPAL is distributed under the terms of GNU General Public License, version 3
(or any later version) as published by the Free Software Foundation. See
gpl.txt for details.

Also see credits.txt for additional credits.

This program does NOT include any code or data files of the original game,
which is proprietary and copyrighted by SoftStar Inc.


COMPILE UNDER WINDOWS
=====================

The following compilers/IDEs are supported under Windows:

1) Microsoft Visual Studio 2013 or higher
2) Dev-C++ 4.9.9.2
3) Open Watcom 1.7

To compile, open the respective project file (sdlpal.sln, sdlpal.dev, or
sdlpal.wpj). You need to have SDL 2.0 development files installed.


COMPILE UNDER GNU/LINUX
=======================

To compile, type:

make

You need to have SDL 2.0 development files installed. The compiled executable
should be generated with the filename 'sdlpal' at the top directory of source
files.

SDLPAL should also be able to compile and run under other UNIX-like systems, 
however it's not tested.


COMPILE UNDER MAC OS X
======================

To compile, open the project Pal.xcodeproj with Xcode, and click Build. You
need to have SDL framework installed at /Library/Frameworks.

The compiled bundle should work as a "universal" binary which works on both
Intel and PowerPC.


CLASSIC BUILD
=============

By default, SDLPAL uses a revised battle system which is more exciting yet
somewhat harder than the original game. If you prefer the traditional
turn-based battle system, uncomment the following line:

//#define PAL_CLASSIC           1

in the file common.h and recompile. This will build a "classic" build which is
100% the same as the original game.


RUNNING THE GAME
================

The data files required for running the game are not included with the source
package due to copyright issues. You must obtain them from the original CD.

To run the game, copy all the files on the original CD to a directory, then
copy the SDLPAL executable to the same directory, and run the executable.

Note that the filenames of data files should be in lower-case under GNU/Linux
(or other UNIX-like operating systems).


CONFIGURE THE GAME
==================

PAL has several variants using different and incompatible resource files. Now
SDLPAL supports several configuration options for supporting such variants.

To set these configuration options, create a file named as 'sdlpal.cfg' (make
sure to use lower-case file name in UNIX-like operating systems) in the game
directory created by the above step. If no configuration file exists, SDLPAL
uses default values that supports the original resources of DOS version.

Please refer the 'sdlpal.cfg.example' in src directory for its format.


-END OF FILE-
