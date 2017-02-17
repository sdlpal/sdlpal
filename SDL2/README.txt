Download SDL 2.0 source code from http://libsdl.org/ and extract here.

Special notes for Windows store app versions (under winrt directory):

To make the screen shot & mp3 functions work properly, you need to modify the SDL's project files within 
the VisualC-WinRT directory (including SDL-WinPhone81.vcxproj, SDL-WinRT81.vcxproj & SDL-UWP.vcxproj)
by manually place the library file 'sdlpal.common.lib' before the 'msvcrt[d].lib'. This work can be done
by adding the following LINK options:

"sdlpal.common.lib" "vccorlib[d].lib" "msvcrt[d].lib" /NODEFAULTLIB:"vccorlib[d].lib" /NODEFAULTLIB:"msvcrt[d].lib"

Note the the optional 'd' postfix is used only in debug builds. Also, library search path for "sdlpal.common.lib"
should be appended to the linker's command line. This work can also be done through the Visual Studio's GUI.
