/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#ifdef __cplusplus
extern "C"
{
#endif

#include "ui.h"

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

#ifdef __cplusplus
}
#endif
#endif
