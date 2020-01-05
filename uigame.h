/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2020, SDLPAL development team.
// All rights reserved.
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

#ifndef UIGAME_H
#define UIGAME_H

#include "common.h"
#include "ui.h"

PAL_C_LINKAGE_BEGIN

VOID
PAL_DrawOpeningMenuBackground(
   VOID
);

INT
PAL_OpeningMenu(
   VOID
);

INT
PAL_SaveSlotMenu(
   WORD        wDefaultSlot
);

WORD
PAL_TripleMenu(
   WORD  wThirdWord
);

BOOL
PAL_ConfirmMenu(
   VOID
);

BOOL
PAL_SwitchMenu(
   BOOL      fEnabled
);

VOID
PAL_InGameMagicMenu(
   VOID
);

VOID
PAL_InGameMenu(
   VOID
);

VOID
PAL_PlayerStatus(
   VOID
);

WORD
PAL_ItemUseMenu(
   WORD           wItemToUse
);

VOID
PAL_BuyMenu(
   WORD           wStoreNum
);

VOID
PAL_SellMenu(
   VOID
);

VOID
PAL_EquipItemMenu(
   WORD           wItem
);

VOID
PAL_QuitGame(
   VOID
);

PAL_C_LINKAGE_END

#endif
