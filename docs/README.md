# SDLPAL

[![GitHub CI](https://github.com/sdlpal/sdlpal/actions/workflows/linux.yml/badge.svg)](https://github.com/sdlpal/sdlpal/actions)
[![Download](https://api-prd.cloudsmith.io/badges/version/sdlpal/sdlpal/raw/MinGW32/latest/x/?render=true) ](https://cloudsmith.io/~sdlpal/repos/sdlpal/groups)

**SDLPAL** is an SDL-based cross-platform reimplementation of the classic Chinese RPG *Legend of Sword and Fairy* (also known as *PAL*).

[Try the online demo\!](https://sdlpal.github.io/demo/sdlpal.html)

Check the link above for a web-based demo of SDLPAL. It should work on most modern browsers (e.g., Google Chrome, Mozilla Firefox, Safari), though compatibility issues may exist on some platforms. To play, please prepare a ZIP file containing the original game resource data.

## Gitee Mirror (Gitee 镜像)

If you experience difficulties downloading code from GitHub in mainland China, please use our [Gitee mirror](https://gitee.com/sdlpal/sdlpal) (updated hourly).

-----

## License

SDLPAL was originally created by [Wei Mingzhi](https://github.com/weimzh/). It is now maintained by the SDLPAL development team. Please see [AUTHORS](https://www.google.com/search?q=AUTHORS) for the full list of contributors.

```text
Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
Copyright (c) 2011-2024, SDLPAL development team.
All rights reserved.
```

SDLPAL is distributed under the terms of the **GNU General Public License, version 3**, as published by the [Free Software Foundation](http://www.fsf.org/). See [LICENSE](https://www.google.com/search?q=LICENSE) for details.

Many concepts in this program are based on documents from the [PAL Research Project](https://github.com/palxex/palresearch), and portions of the code are based on work by Baldur and [louyihua](https://github.com/louyihua).

This program makes extensive use of the following libraries:

  * **SDL** / **SDL\_mixer**
  * **libmad**, **libogg** & **libvorbis**, **libopus** & **opusfile**
  * **FLTK**, **TinySoundFont**
  * OPL player from **Adplug**
  * OPL emulation cores from **DOSBOX**, **MAME**, and **Chocolate Doom** projects
  * Audio resampler from **foo\_input\_adplug**
  * AVI player from **FFmpeg**
  * Image decoder from **stb**

This program does **NOT** include any code or data files from the original game, which are proprietary and copyrighted by **SoftStar Inc.**

We recommend purchasing the original game from [Steam](https://store.steampowered.com/app/1546570/Sword_and_Fairy/) to obtain the required data files.

-----

## FAQ

Please refer to the [Wiki](https://www.google.com/search?q=../../wiki).

-----

## Building the Game

SDLPAL currently supports the following platforms:

  * **Desktop:** Windows (Desktop & UWP), GNU/Linux, and macOS.
  * **Mobile:** Android, iOS, and Windows Phone/Mobile.
  * **Consoles:** 3DS, Wii, PSP, and others (some may not be actively maintained).

For build environment inspiration, you can review our [CI scripts](https://www.google.com/search?q=.travis.yml).

### General Build Steps

1.  **Clone the repository:**
    ```shell
    $ git clone https://github.com/sdlpal/sdlpal.git
    $ cd sdlpal
    ```
2.  **Update submodules:**
    ```shell
    $ git submodule update --init --recursive
    ```
3.  **Follow platform-specific instructions:**

### Windows

  * **Visual Studio:** - For **Desktop**, open `win32/sdlpal.sln` in Visual Studio 2017 or later.
      - For **UWP**, open `winrt/SDLPal.UWP.sln`.
  * **MinGW:**
      - **Windows CMD:** `cd win32 && make -f Makefile.mingw`
      - **MSYS:** `cd win32 && make`
      - **Linux Cross-compile:** `cd win32 && make HOST=i686-w64-mingw32-` (or `x86_64` for 64-bit).

### GNU/Linux or Unix

Run `cd unix && make`. Ensure SDL 2.0 development files are installed. The executable `sdlpal` will be generated in the current directory. By default, it uses FLTK for the launch GUI; to disable this, define `PAL_NO_LAUNCH_UI` in the `Makefile`.

### macOS

Open `Pal.xcodeproj` with Xcode and build. Ensure the SDL framework is installed in `/Library/Frameworks`.

### iOS

Install dependencies via CocoaPods, then open `ios/SDLPal/SDLPal.xcworkspace` in Xcode.

```shell
$ cd iOS/SDLPAL
$ pod install
```

### Android

Open the `android` directory in **Android Studio** and select `Make Project`.

  * **Note:** `android/app/src/main/java/org/libsdl/app` is a symlink. Ensure your Git client handles symbolic links correctly.
  * **Tip:** On Windows, keep the repository path short (e.g., at the drive root) to avoid path length issues.

-----

## Running the Game

Resource files are not included. You must copy the original game data files into a directory, place the built SDLPAL executable in that same directory, and then run it.

**Important:** On case-sensitive filesystems (like Linux/macOS), ensure all game data filenames are in **lower-case**.

### MIDI Support

MIDI support varies by platform. It is officially supported on **Windows, Android, iOS, and macOS**. GNU/Linux support is preliminary and requires the `timidity` package. Other platforms do not currently support MIDI.

-----

## Configuration

SDLPAL supports various versions of the original game through configuration.

1.  **GUI:** On the first launch, a configuration window will appear. You can access it later via the in-game menu or after a crash. (Currently unavailable on macOS).
2.  **Manual:** Create a `sdlpal.cfg` file in the game directory. Refer to [sdlpal.cfg.example](https://www.google.com/search?q=sdlpal.cfg.example) for the format.

-----

## Contributing & Issues

  * **Issues:** Report bugs via GitHub Issues in English or Chinese.
  * **Contributions:** PRs are welcome\! Code must be licensed under the GPL. To maintain code quality, please include **unit tests** for new features. See the [testing guide](https://github.com/sdlpal/sdlpal/tree/master/tests) for details.
