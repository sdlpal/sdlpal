### Download SDL 2.0 source code from https://www.libsdl.org/download-2.0.php and extract here.

### Special notes for compiling Windows store app versions

To make the screen-shot & mp3 functions work properly, you need to modify the corresponding SDL's project file within the *`VisualC-WinRT`* directory (including *`SDL-WinPhone81.vcxproj`*, *`SDL-WinRT81.vcxproj`* & *`SDL-UWP.vcxproj`*) by manually place the library file *`sdlpal.common.lib`* before the *`msvcrt[d].lib`*. This work can be done by adding the following LINK options for *`Release`* builds:
```
"sdlpal.common.lib" "vccorlib.lib" "msvcrt.lib" /NODEFAULTLIB:"vccorlib.lib" /NODEFAULTLIB:"msvcrt.lib"
```
Or by adding the following LINK options for *`Debug`* builds:
```
"sdlpal.common.lib" "vccorlibd.lib" "msvcrtd.lib" /NODEFAULTLIB:"vccorlibd.lib" /NODEFAULTLIB:"msvcrtd.lib"
```
Also, library search path for *`sdlpal.common.lib`* should be appended to the linker's command line. This work can also be done through the Visual Studio's GUI.
