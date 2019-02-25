SDLPAL
======
[![Travis CI](https://travis-ci.org/sdlpal/sdlpal.svg?branch=master)](https://travis-ci.org/sdlpal/sdlpal)
[![AppVeyor](https://ci.appveyor.com/api/projects/status/github/sdlpal/sdlpal?branch=master&svg=true)](https://ci.appveyor.com/project/palxex/sdlpal-itfml)
[ ![Download](https://api.bintray.com/packages/sdlpal/nightly/master/images/download.svg) ](https://bintray.com/sdlpal/nightly/master/_latestVersion)

***SDLPAL*** is an SDL-based open-source cross-platform reimplementation of the classic Chinese RPG game *Xiān jiàn Qí Xiá Zhuàn (Chinese: 仙剑奇侠传/仙劍奇俠傳)* (also known as *Chinese Paladin* or *Legend of Sword and Fairy*, or *PAL* for short).

[![Google Play](https://sdlpal.github.io/images/googleplay.png)](https://play.google.com/store/apps/details?id=com.sdlpal.sdlpal)

LICENSE
=======

SDLPAL is originally created by [Wei Mingzhi](https://github.com/CecilHarvey) from 2009. Now it is owned by the SDLPAL development team. Please see [AUTHORS](AUTHORS) for full author list.
```
Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
Copyright (c) 2011-2018, SDLPAL development team.
All rights reserved.
```
SDLPAL is distributed under the terms of GNU General Public License, version 3 (or any later version) as published by the Free Software Foundation. See [LICENSE](LICENSE) for details.

Many of the ideas of this program are based on documents from [PAL Research Project](https://github.com/palxex/palresearch), and portions of the code are based on the work done by Baldur and [louyihua](https://github.com/louyihua).

This program made extensive use of the following libraries:
* [SDL](http://www.libsdl.org/)
* [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/)
* [libmad](http://www.underbit.com/products/mad/)
* [libogg & libvorbis](http://www.vorbis.com/)
* [FLTK](http://www.fltk.org)
* OPL player from [Adplug](http://adplug.sourceforge.net/)
* OPL emulation cores from [DOSBOX project](http://www.dosbox.com), [MAME project](http://mamedev.org/) and [Chocolate Doom project](https://github.com/chocolate-doom/chocolate-doom)
* Audio resampler from [foo_input_adplug](https://www.foobar2000.org/components/view/foo_input_adplug)
* AVI player from [ffmpeg](https://ffmpeg.org/)
* Image decoder from [stb](https://github.com/nothings/stb)

This program does **NOT** include any code or data files of the original game, which are proprietary and copyrighted by [SoftStar](http://www.softstar.com.tw/) Inc.

## FAQ

See [wiki](../../wiki)

## Chat Room

Want to chat with other members of the SDLPal community?

We have a Gitter Room which you can join below.

[![Join the chat at https://gitter.im/sdlpal/sdlpal](https://badges.gitter.im/sdlpal/sdlpal.svg)](https://gitter.im/sdlpal/sdlpal?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Building the game
=================

Currently, SDLPAL supports the following platforms:
* Desktop platforms: *Windows* (both *Windows Desktop* and *Universal Windows Platform*), *Linux* and *macOS*
* Mobile platforms: *Android*, *iOS* and *Windows*
* Home consoles: *3DS*, *WII*, *PSP* and others (some of them are not actively maintained now)

Before start building, you MUST READ [Our travis scripts](.travis.yml) to see how automated building process works.

To build SDLPAL, there are three major steps you need to do:

1. Clone the source code from Github into your local folder using `git` or through corresponding GUI:
```shell
cd <parent-path-of-sdlpal>
git clone https://github.com/sdlpal/sdlpal.git
```
2. Update all the submodules used by SDLPAL using `git submodule` or through corresponding GUI:
```shell
cd <parent-path-of-sdlpal>/sdlpal
git submodule update --init --recursive
```
3. Follow the platform-specific steps listed below.

Build for Windows NT
-------

We dropped support of SDL1 for Windows NT builds of SDLPal, hence the minimum system requirements of Windows NT 5.1 (i.e. Windows XP, Windows Fundamentals of Legacy PCs, Windows Embedded POS-Ready 2009, and Windows Server 2003 for x86 and amd64 architectures. IA64 is not supported). **Note:** If this project implements WASAPI and drops DirectSound support in the future, then the minimum system requirements will be Windows NT 6.0 (Windows Vista and Server 2008 R1).

### Visual Studio

To build SDLPAL as a Windows **desktop** app, you can use ***Microsoft Visual Studio 2017*** to open the solution file *`sdlpal.sln`* under the *`win32`* directory. Note that if you don't want to install  Visual Studio (someone may hate that it takes too much disk spaces), you can try compiling using MinGW instead.

To build SDLPAL as a **Universal Windows Platform** app, you can use ***Microsoft Visual Studio 2017*** to open the solution file *`SDLPal.UWP.sln`* under the *`winrt`* directory.

### MinGW

To build SDLPAL as a Windows **desktop** app, you can also use ***MinGW***. Steps for building under MinGW varies depends on the compiling environment you have:

* If you need to compile SDLPAL under **Windows** shell environment, please go to the root of the source code tree and type:
```cmd
C:\sdlpal> cd win32
C:\sdlpal> make -f Makefile.mingw
```

* If you need to compile SDLPAL under **msys** shell environment, please go to the root of the source code tree and type:
```bash
cd win32
make
```

Note: Below are MinGW builds under macOS / Linux build environments. Before building them, you need to put the MinGW support files of SDL2 to the correct places. We don't explain too much about it, and you have to study the `.travis.yml` to see how it works.

* If you need to cross-compile SDLPAL under **Linux** shell environment, please go to the root of the source code tree and type:
```bash
cd win32
# This builds a 32-bit executable.
make HOST=i686-w64-mingw32-
# This builds a 64-bit executable.
make HOST=x86_64-w64-mingw32-
```

* If you need to cross-compile SDLPAL under **macOS** shell environment (suppose that you have installed MinGW64 through MacPorts), please go to the root of the source code tree and type:
```bash
cd win32
# This builds a 32-bit executable.
export PATH=/opt/local/i686-w64-mingw32/bin:$PATH && make HOST=i686-w64-mingw32-
# This builds a 64-bit executable.
export PATH=/opt/local/x86_64-w64-mingw32/bin:$PATH && make HOST=x86_64-w64-mingw32-
```

Build for Linux or Unix (except macOS)
-------------

To build the game, please go to the root of the source code tree and type:
```shell
cd unix
make
```
You also need to have SDL 2.0 development files installed in the system. The compiled executable should be generated with the filename *`sdlpal`* at the current directory. By default, SDLPAL uses the FLTK library to provide setting GUI at launch. If you do not want to use the library, please define the macro `PAL_NO_LAUNCH_UI` in the `Makefile`. SDLPAL should also be able to compile and run under other Unix-like systems, however it's not tested.

Build for macOS (OS X)
------------

Don't forget to install the latest Xcode (or one major version behind) from the Mac App Store. If you failed to perform the online installation, you can go to [Apple Developer](https://developer.apple.com/download/more/) website to download them manually. Run Xcode at least once (to accept the EULA of this GUI application).

Also, we need `wget` to finish the next step because `curl` doesn't work well. You can use either `sudo port install wget` or `brew install wget` to install it, depending on which package manager you prefer. You may also just download the source code of wget and compile it + `make install`.

Currently, if built on macOS 10.14 Mojave with Xcode 10.x, the generated `Pal.app` bundle has to be manually configured to run in low-resolution mode. Otherwise, you may see the window contents get zoomed in 2x size. Those builts on Travis were built with Xcode 9.x running on macOS 10.13 High Sierra, and they behave well on macOS 10.14 Mojave except the color of the title bar (i.e. no support of Dark Mode).

At this moment, if you don't want to use the blury low-resolution mode, you can use the SDKs for older macOS releases. They are extractable from Commandline Tools of older Xcode versions. Theoreotically, the SDK you use secures the compatibility between what you built and the target system which the SDK is made for. In this case we use the macOS 10.11 SDK for the sake of the compatibility with macOS 10.11-10.13. We can install this SDK by running the following bash command:
```bash
wget -qO- https://github.com/alexey-lysiuk/macos-sdk/releases/download/10.11/MacOSX10.11.tar.bz2 | sudo tar xvz - -C /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
```

After that, you need to accept the license of it by running this bash command (then press PageDown to the end and follow the instructions printed on your screen):
```bash
sudo xcodebuild -license
```

Then you have to build the latest SDL2 framework (customized in the source code tree) installed at *`/Library/Frameworks`*. You have to build it by yourself.

#### // Compiling  the customized SDL2 library.

If using regular (officially released) SDL2 library, then the built SDLPal cannot function well in anything related to the GLSL. If you don't want to enable GLSL in SDLPal, you can skip this step.

Note: At least since 2.0.8, an SDL2 library must be compiled with at least macOS 10.11 SDK. SDKs for non-Metal macOS releases (macOS 10.05-10.10) will fail the compilation (due to the lack of Metal.framework). At this moment we suggest build only amd64 (x86_64) builds, and this means that we don't support those mac models using Core Duo processors (their CPUs only support 32-bit).

Go to the root of the source code tree and run the following bash commands (using macOS 10.11 SDK as example):

```bash
cd 3rd/SDL/Xcode/SDL

# Build the customized SDL2 framework.
export cc="/usr/bin/gcc" && xcodebuild -project SDL.xcodeproj -scheme Framework -configuration Release CONFIGURATION_BUILD_DIR="./" -arch x86_64 -sdk macosx10.11 build

# Removing the existed SDL2 framework installed in your system.
sudo rm -rf /Library/Frameworks/SDL2.framework

# Applying the newly built SDL2 framework.
sudo mv -f SDL2.framework /Library/Frameworks/
```

#### // Compiling the SDLPal application bundle for non-Metal macOS Releases.

This build requires macOS 10.07 SDK installed in Xcode 5 (or Xcode 6) running on macOS 10.8-10.10 (compiling environment). Older SDKs won't built. Newer SDKs may drop support of macOS 10.05-10.06 Leopards. If building using macOS 10.14 SDK, then it only runs on at least macOS 10.14 Mojave.

Due to some reasons, this build doesn't support GLSL functionalities and some other functions like OGG music support, etc. Hence, you may use official releases of SDL2 framework instead of the customized one built above.

Run the following bash command to install macOS 10.07 SDK:
```
wget -qO- https://github.com/alexey-lysiuk/macos-sdk/releases/download/10.7/MacOSX10.7.tar.bz2 | sudo tar xvz - -C /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
```

Go to the root of the source code tree and run the following bash commands:

```bash
cd macos
export cc="/usr/bin/gcc" && xcodebuild -project Pal_PreMetal.xcodeproj -configuration Release CONFIGURATION_BUILD_DIR="./" -arch x86_64 -sdk macosx10.7 build && rm -rf ~/Library/Developer/
```
Then you can see the generated app bundle in the `macos` directory.

#### // Compiling the SDLPal application bundle for macOS 10.11 and later.

This requires at least macOS 10.11 SDK (or won't built due to the lack of Metal framework). If building using macOS 10.14 SDK, then it only runs on at least macOS 10.14 Mojave.

Note: Using official releases of SDL2 framework (in lieu of the customized one built above) only loses support of GLSL.

Go to the root of the source code tree and run the following bash commands:

```bash
cd macos
export cc="/usr/bin/gcc" && xcodebuild -project Pal.xcodeproj -scheme SDLPal -configuration Release CONFIGURATION_BUILD_DIR="./" -arch x86_64 -sdk macosx10.11 build
```
Then you can see the generated app bundle in the `macos` directory.

#### // Compiling the SDLPal application bundle for macOS 10.04-10.05 PPC.

We don't support PowerPC platforms anymore due to the raised difficulty of finding a testing machine, and our limited human resources shall focus on latest builds serving the most-recent three major releases of macOS. Still, the `Pal_SDL12.xcodeproj` is there for anyone who is eager to run SDLPal on a Power-PC macOS platform (or Intel mac) running macOS 10.04-10.05 systems, and we don't guarantee whether it works or not in the future. Also, SDL1 framework introduces reduced video performances.


iOS
---

To compile, please first install dependencies via CocoaPods following the above instruments, then open the project *`ios/SDLPal/SDLPal.xcworkplace`* with `Xcode`, and click Build.
```shell
cd iOS/SDLPAL
sudo gem install cocoapods # ONLY need do once on one machine
pod install # ONLY need do once in one repository
```

Android
-------

To build the game, open the `android` directory through ***Android Studio***, and click `Make Project`.

Nintendo 3DS
------------

To build the game, please go to the root of the source code tree and type:
```shell
cd 3ds
make
make cia
```
You need to have *DevkitPro ARM* and *SDL 1.2* for 3DS portlib installed. Creating a CIA package is not required to play the game, but in order to to that, a seperate *makerom* tool is required. The compiled executable should be generated with the filename *`sdlpal`* at the current directory.
 
Nintendo Wii
------------

To build the game, please go to the root of the source code tree and type:
```shell
cd wii
make
```
You need to have *DevkitPro PPC* and *SDL 1.2* for Wii portlib installed.

Other platforms
---------------

To be written.


Choosing the battle system
==========================

By default, SDLPAL builds a *"classic"* turn-based battle system which is designed to be 100% the same as the original game.

SDLPAL also provides a revised battle system (***deprecated*** and will be removed in future) which is more exciting yet somewhat harder than the original game. If you prefer this battle system, please define the macro `ENABLE_REVISIED_BATTLE` in *`Makefile`* or in *`common.h`* and recompile the project.


Running the game
================

The data files required for running the game are not included with the source package due to copyright issues, so you need to obtain them from a licensed game copy before you can run the game.

To run the game, copy all the files in the original game CD to a directory, then copy the built SDLPAL executable to the same directory, and run the executable.

Note that the filenames of data files should be all in lower-case under systems that use case-sensitive filesystems such as Linux or other Unix-like operating systems.

If you prefer using MIDI as background music source, please note that the MIDI playing feature is not yet complete under every supported platform. Currently, **offical** support is provided under *Windows Desktop*, *Universal Windows Platform*, *Android*, *iOS* and *macOS*. There is also a preliminary support for *Linux* that relys upon package *timidity*. Other platforms do not support playing MIDI for now.


Configuring the game
====================

PAL has several variants using different and incompatible resource files, and SDLPAL supports several configuration options for supporting such variants. The default values are used to support the resources from original DOS version. If you want to change these configurations, you have two options: through the configuration GUI or by manipulating the configuration file *`sdlpal.cfg`* manually.

GUI
---
The configuration GUI provides options for you to change the most common configuration options. If you launch SDLPAL for the first time, it will bring you to the configuration GUI by default. Once you have saved configurations from the GUI, the GUI will not to show again on subsequent launches. However, you have the opportunity to bring the GUI back on fatal game program errors or through the in-game system menu.

Currently, the configuration GUI is available under the following platforms:
* Desktop platforms: *Windows* (both *Windows Desktop* and *Universal Windows Platform*) and *Linux*
* Mobile platforms: *Android*, *Windows* and *iOS*

Configuration GUIs for *macOS* is still unavailable for now and we welcome contributions to implement it.

Manually
--------
To set the configuration options manually, create a file named as *`sdlpal.cfg`* (make sure to use lower-case file name in case-sensitive filesystems) in the game directory created by the above step. Please refer to the [example file](sdlpal.cfg.example) for format specfication.


Reporting issues
================

If you find any issues of SDLPAL, please feel free to report them to the development team through Github's issue tracking system using either English or Chinese.


Contributing to the game
========================

Any original code & documentation contributions are welcomed as long as the contributed code & documentation is licensed under GPL. You can use Github's pull request system to submit your changes to the main repository here. But remember, as a step to keep the quality of code, you should write corresponding unit tests before your changes can be merged. The guidance of writting unit tests can be found [here](/tests).
