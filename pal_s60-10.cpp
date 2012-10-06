//
// Copyright (c) 2009, netwan. All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef __SYMBIAN32__

#include <eikstart.h>
#include <sdlmain.h>
#include <sdlepocapi.h>

/*
This file demonstrates how to change CSDL when using sdlexe.lib - i.e. if default flags are not
ok  - you dont have to use CSDL API directly, you can write this file add add it in to your S60 SDL
compilation. Then you dont statically link  sdlmain.lib or  sdlmaint.lib libraries
*/

GLREF_C TInt E32Main()
    {
#ifdef __S60_5X__
    return SDLEnv::SetMain(SDL_main,CSDL::EEnableFocusStop
                    | CSDL::EAutoOrientation /*| CSDL::EAllowImageResizeKeepRatio | CSDL::EDrawModeGdi*/,
             NULL/*, SDLEnv::EParamQuery | SDLEnv::EEnableVirtualMouseMoveEvents*/);
#else
    return SDLEnv::SetMain(SDL_main,CSDL::EEnableFocusStop
                | CSDL::EAutoOrientation | CSDL::EAllowImageResizeKeepRatio /*| CSDL::EDrawModeGdi*/,
         NULL, /*SDLEnv::EParamQuery |*/ SDLEnv::EEnableVirtualMouseMoveEvents);
#endif
    }

#endif
