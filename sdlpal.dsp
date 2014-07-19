# Microsoft Developer Studio Project File - Name="sdlpal" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=sdlpal - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sdlpal.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sdlpal.mak" CFG="sdlpal - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sdlpal - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "sdlpal - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sdlpal - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Op /I "d:\sdl\include" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /machine:I386 /libpath:"d:\sdl\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "sdlpal - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "d:\sdl\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"d:\sdl\lib\\"

!ENDIF 

# Begin Target

# Name "sdlpal - Win32 Release"
# Name "sdlpal - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\battle.c
# End Source File
# Begin Source File

SOURCE=.\ending.c
# End Source File
# Begin Source File

SOURCE=.\fight.c
# End Source File
# Begin Source File

SOURCE=.\font.c
# End Source File
# Begin Source File

SOURCE=.\game.c
# End Source File
# Begin Source File

SOURCE=.\getopt.c
# End Source File
# Begin Source File

SOURCE=.\global.c
# End Source File
# Begin Source File

SOURCE=.\input.c
# End Source File
# Begin Source File

SOURCE=.\itemmenu.c
# End Source File
# Begin Source File

SOURCE=.\magicmenu.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\map.c
# End Source File
# Begin Source File

SOURCE=.\midi.c
# End Source File
# Begin Source File

SOURCE=.\palcommon.c
# End Source File
# Begin Source File

SOURCE=.\palette.c
# End Source File
# Begin Source File

SOURCE=.\play.c
# End Source File
# Begin Source File

SOURCE=.\res.c
# End Source File
# Begin Source File

SOURCE=.\rixplay.cpp
# End Source File
# Begin Source File

SOURCE=.\rngplay.c
# End Source File
# Begin Source File

SOURCE=.\scene.c
# End Source File
# Begin Source File

SOURCE=.\script.c
# End Source File
# Begin Source File

SOURCE=.\sound.c
# End Source File
# Begin Source File

SOURCE=.\text.c
# End Source File
# Begin Source File

SOURCE=.\ui.c
# End Source File
# Begin Source File

SOURCE=.\uibattle.c
# End Source File
# Begin Source File

SOURCE=.\uigame.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\video.c
# End Source File
# Begin Source File

SOURCE=.\yj1.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ascii.h
# End Source File
# Begin Source File

SOURCE=.\battle.h
# End Source File
# Begin Source File

SOURCE=.\big5font.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\ending.h
# End Source File
# Begin Source File

SOURCE=.\fight.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\gbfont.h
# End Source File
# Begin Source File

SOURCE=.\getopt.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\itemmenu.h
# End Source File
# Begin Source File

SOURCE=.\magicmenu.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\map.h
# End Source File
# Begin Source File

SOURCE=.\midi.h
# End Source File
# Begin Source File

SOURCE=.\palcommon.h
# End Source File
# Begin Source File

SOURCE=.\palette.h
# End Source File
# Begin Source File

SOURCE=.\play.h
# End Source File
# Begin Source File

SOURCE=.\res.h
# End Source File
# Begin Source File

SOURCE=.\rixplay.h
# End Source File
# Begin Source File

SOURCE=.\rngplay.h
# End Source File
# Begin Source File

SOURCE=.\scene.h
# End Source File
# Begin Source File

SOURCE=.\script.h
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\text.h
# End Source File
# Begin Source File

SOURCE=.\ui.h
# End Source File
# Begin Source File

SOURCE=.\uibattle.h
# End Source File
# Begin Source File

SOURCE=.\uigame.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\video.h
# End Source File
# End Group
# Begin Group "adplug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\adplug\binfile.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\binfile.h
# End Source File
# Begin Source File

SOURCE=.\adplug\binio.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\binio.h
# End Source File
# Begin Source File

SOURCE=.\adplug\demuopl.h
# End Source File
# Begin Source File

SOURCE=.\adplug\dosbox_opl.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\dosbox_opl.h
# End Source File
# Begin Source File

SOURCE=.\adplug\emuopl.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\emuopl.h
# End Source File
# Begin Source File

SOURCE=.\adplug\fmopl.c
# End Source File
# Begin Source File

SOURCE=.\adplug\fmopl.h
# End Source File
# Begin Source File

SOURCE=.\adplug\fprovide.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\fprovide.h
# End Source File
# Begin Source File

SOURCE=.\adplug\opl.h
# End Source File
# Begin Source File

SOURCE=.\adplug\player.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\player.h
# End Source File
# Begin Source File

SOURCE=.\adplug\rix.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\rix.h
# End Source File
# Begin Source File

SOURCE=.\adplug\surroundopl.cpp
# End Source File
# Begin Source File

SOURCE=.\adplug\surroundopl.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sdlpal.ico
# End Source File
# Begin Source File

SOURCE=.\sdlpal.rc
# End Source File
# End Group
# Begin Group "native_midi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\native_midi\native_midi.h
# End Source File
# Begin Source File

SOURCE=.\native_midi\native_midi_common.c
# End Source File
# Begin Source File

SOURCE=.\native_midi\native_midi_common.h
# End Source File
# Begin Source File

SOURCE=.\native_midi\native_midi_win32.c
# End Source File
# End Group
# Begin Group "libmad"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libmad\bit.c
# End Source File
# Begin Source File

SOURCE=.\libmad\bit.h
# End Source File
# Begin Source File

SOURCE=.\libmad\D.dat
# End Source File
# Begin Source File

SOURCE=.\libmad\decoder.c
# End Source File
# Begin Source File

SOURCE=.\libmad\decoder.h
# End Source File
# Begin Source File

SOURCE=.\libmad\fixed.c
# End Source File
# Begin Source File

SOURCE=.\libmad\fixed.h
# End Source File
# Begin Source File

SOURCE=.\libmad\frame.c
# End Source File
# Begin Source File

SOURCE=.\libmad\frame.h
# End Source File
# Begin Source File

SOURCE=.\libmad\huffman.c
# End Source File
# Begin Source File

SOURCE=.\libmad\huffman.h
# End Source File
# Begin Source File

SOURCE=.\libmad\imdct_s.dat
# End Source File
# Begin Source File

SOURCE=.\libmad\layer12.c
# End Source File
# Begin Source File

SOURCE=.\libmad\layer12.h
# End Source File
# Begin Source File

SOURCE=.\libmad\layer3.c
# End Source File
# Begin Source File

SOURCE=.\libmad\layer3.h
# End Source File
# Begin Source File

SOURCE=.\libmad\libmad_config.h
# End Source File
# Begin Source File

SOURCE=.\libmad\libmad_global.h
# End Source File
# Begin Source File

SOURCE=.\libmad\mad.h
# End Source File
# Begin Source File

SOURCE=.\libmad\music_mad.c
# End Source File
# Begin Source File

SOURCE=.\libmad\music_mad.h
# End Source File
# Begin Source File

SOURCE=.\libmad\qc_table.dat
# End Source File
# Begin Source File

SOURCE=.\libmad\rq_table.dat
# End Source File
# Begin Source File

SOURCE=.\libmad\sf_table.dat
# End Source File
# Begin Source File

SOURCE=.\libmad\stream.c
# End Source File
# Begin Source File

SOURCE=.\libmad\stream.h
# End Source File
# Begin Source File

SOURCE=.\libmad\synth.c
# End Source File
# Begin Source File

SOURCE=.\libmad\synth.h
# End Source File
# Begin Source File

SOURCE=.\libmad\timer.c
# End Source File
# Begin Source File

SOURCE=.\libmad\timer.h
# End Source File
# End Group
# End Target
# End Project
