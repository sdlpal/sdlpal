/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2008, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//
// Portions based on PALx Project by palxex.
// Copyright (c) 2006-2008, Pal Lockheart <palxex@gmail.com>.
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

#include "main.h"

#define WORD_LENGTH      10

#define   FONT_COLOR_DEFAULT        0x4F
#define   FONT_COLOR_YELLOW         0x2D
#define   FONT_COLOR_RED            0x1A
#define   FONT_COLOR_CYAN           0x8D
#define   FONT_COLOR_CYAN_ALT       0x8C

BOOL      g_fUpdatedInBattle      = FALSE;

static const char g_rgszAdditionalWords[][WORD_LENGTH + 1] = {
   {0xBE, 0xD4, 0xB0, 0xAB, 0xB3, 0x74, 0xAB, 0xD7, 0x00, 0x00, 0x00}, // Battle Speed
   {0xA4, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 1
   {0xA4, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 2
   {0xA4, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 3
   {0xA5, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 4
   {0xA4, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 5
};

typedef struct tagTEXTLIB
{
   LPBYTE          lpWordBuf;
   LPBYTE          lpMsgBuf;
   LPDWORD         lpMsgOffset;

   int             nWords;
   int             nMsgs;

   int             nCurrentDialogLine;
   BYTE            bCurrentFontColor;
   PAL_POS         posIcon;
   PAL_POS         posDialogTitle;
   PAL_POS         posDialogText;
   BYTE            bDialogPosition;
   BYTE            bIcon;
   int             iDelayTime;
   BOOL            fUserSkip;
   BOOL            fPlayingRNG;

   BYTE            bufDialogIcons[282];
} TEXTLIB, *LPTEXTLIB;

static TEXTLIB         g_TextLib;

INT
PAL_InitText(
   VOID
)
/*++
  Purpose:

    Initialize the in-game texts.

  Parameters:

    None.

  Return value:

    0 = success.
    -1 = memory allocation error.

--*/
{
   FILE       *fpMsg, *fpWord;
   int         i;

   //
   // Open the message and word data files.
   //
   fpMsg = UTIL_OpenRequiredFile("m.msg");
   fpWord = UTIL_OpenRequiredFile("word.dat");

   //
   // See how many words we have
   //
   fseek(fpWord, 0, SEEK_END);
   i = ftell(fpWord);

   //
   // Each word has 10 bytes
   //
   g_TextLib.nWords = (i + (WORD_LENGTH - 1)) / WORD_LENGTH;

   //
   // Read the words
   //
   g_TextLib.lpWordBuf = (LPBYTE)malloc(i);
   if (g_TextLib.lpWordBuf == NULL)
   {
      fclose(fpWord);
      fclose(fpMsg);
      return -1;
   }
   fseek(fpWord, 0, SEEK_SET);
   fread(g_TextLib.lpWordBuf, i, 1, fpWord);

   //
   // Close the words file
   //
   fclose(fpWord);

   //
   // Read the message offsets. The message offsets are in SSS.MKF #3
   //
   i = PAL_MKFGetChunkSize(3, gpGlobals->f.fpSSS) / sizeof(DWORD);
   g_TextLib.nMsgs = i - 1;

   g_TextLib.lpMsgOffset = (LPDWORD)malloc(i * sizeof(DWORD));
   if (g_TextLib.lpMsgOffset == NULL)
   {
      free(g_TextLib.lpWordBuf);
      fclose(fpMsg);
      return -1;
   }

   PAL_MKFReadChunk((LPBYTE)(g_TextLib.lpMsgOffset), i * sizeof(DWORD), 3,
      gpGlobals->f.fpSSS);

   //
   // Read the messages.
   //
   fseek(fpMsg, 0, SEEK_END);
   i = ftell(fpMsg);

   g_TextLib.lpMsgBuf = (LPBYTE)malloc(i);
   if (g_TextLib.lpMsgBuf == NULL)
   {
      free(g_TextLib.lpMsgOffset);
      free(g_TextLib.lpWordBuf);
      fclose(fpMsg);
      return -1;
   }

   fseek(fpMsg, 0, SEEK_SET);
   fread(g_TextLib.lpMsgBuf, 1, i, fpMsg);

   fclose(fpMsg);

   g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
   g_TextLib.bIcon = 0;
   g_TextLib.posIcon = 0;
   g_TextLib.nCurrentDialogLine = 0;
   g_TextLib.iDelayTime = 3;
   g_TextLib.posDialogTitle = PAL_XY(12, 8);
   g_TextLib.posDialogText = PAL_XY(44, 26);
   g_TextLib.bDialogPosition = kDialogUpper;
   g_TextLib.fUserSkip = FALSE;

   PAL_MKFReadChunk(g_TextLib.bufDialogIcons, 282, 12, gpGlobals->f.fpDATA);

   return 0;
}

VOID
PAL_FreeText(
   VOID
)
/*++
  Purpose:

    Free the memory used by the texts.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (g_TextLib.lpMsgBuf != NULL)
   {
      free(g_TextLib.lpMsgBuf);
      g_TextLib.lpMsgBuf = NULL;
   }

   if (g_TextLib.lpMsgOffset != NULL)
   {
      free(g_TextLib.lpMsgOffset);
      g_TextLib.lpMsgOffset = NULL;
   }

   if (g_TextLib.lpWordBuf != NULL)
   {
      free(g_TextLib.lpWordBuf);
      g_TextLib.lpWordBuf = NULL;
   }
}

LPCSTR
PAL_GetWord(
   WORD       wNumWord
)
/*++
  Purpose:

    Get the specified word.

  Parameters:

    [IN]  wNumWord - the number of the requested word.

  Return value:

    Pointer to the requested word. NULL if not found.

--*/
{
   static char buf[WORD_LENGTH + 1];

   if (wNumWord >= PAL_ADDITIONAL_WORD_FIRST)
   {
      return g_rgszAdditionalWords[wNumWord - PAL_ADDITIONAL_WORD_FIRST];
   }

   if (wNumWord >= g_TextLib.nWords)
   {
      return NULL;
   }

   memcpy(buf, &g_TextLib.lpWordBuf[wNumWord * WORD_LENGTH], WORD_LENGTH);
   buf[WORD_LENGTH] = '\0';

   //
   // Remove the trailing spaces
   //
   trim(buf);

   if ((strlen(buf) & 1) != 0 && buf[strlen(buf) - 1] == '1')
   {
      buf[strlen(buf) - 1] = '\0';
   }

   return buf;
}

LPCSTR
PAL_GetMsg(
   WORD       wNumMsg
)
/*++
  Purpose:

    Get the specified message.

  Parameters:

    [IN]  wNumMsg - the number of the requested message.

  Return value:

    Pointer to the requested message. NULL if not found.

--*/
{
   static char    buf[256];
   DWORD          dwOffset, dwSize;

   if (wNumMsg >= g_TextLib.nMsgs)
   {
      return NULL;
   }

   dwOffset = SWAP32(g_TextLib.lpMsgOffset[wNumMsg]);
   dwSize = SWAP32(g_TextLib.lpMsgOffset[wNumMsg + 1]) - dwOffset;
   assert(dwSize < 255);

   memcpy(buf, &g_TextLib.lpMsgBuf[dwOffset], dwSize);
   buf[dwSize] = '\0';

   return buf;
}

VOID
PAL_DrawText(
   LPCSTR     lpszText,
   PAL_POS    pos,
   BYTE       bColor,
   BOOL       fShadow,
   BOOL       fUpdate
)
/*++
  Purpose:

    Draw text on the screen.

  Parameters:

    [IN]  lpszText - the text to be drawn.

    [IN]  pos - Position of the text.

    [IN]  bColor - Color of the text.

    [IN]  fShadow - TRUE if the text is shadowed or not.

    [IN]  fUpdate - TRUE if update the screen area.

  Return value:

    None.

--*/
{
   SDL_Rect   rect, urect;
   WORD       wChar;

   rect.x = PAL_X(pos);
   rect.y = PAL_Y(pos);

   urect.x = rect.x;
   urect.y = rect.y;
#ifdef PAL_WIN95
   urect.h = 17;
#else
   urect.h = 16;
#endif
   urect.w = 0;

   while (*lpszText)
   {
      //
      // Draw the character
      //
      if (*lpszText & 0x80)
      {
         //
         // Chinese Character
         //
         wChar = SWAP16(((LPBYTE)lpszText)[0] | (((LPBYTE)lpszText)[1] << 8));
         if (fShadow)
         {
            PAL_DrawCharOnSurface(wChar, gpScreen, PAL_XY(rect.x + 1, rect.y + 1), 0);
            PAL_DrawCharOnSurface(wChar, gpScreen, PAL_XY(rect.x + 1, rect.y), 0);
         }
         PAL_DrawCharOnSurface(wChar, gpScreen, PAL_XY(rect.x, rect.y), bColor);
         lpszText += 2;
         rect.x += 16;
         urect.w += 16;
      }
      else
      {
         //
         // ASCII character
         //
         if (fShadow)
         {
            PAL_DrawASCIICharOnSurface(*lpszText, gpScreen, PAL_XY(rect.x + 1, rect.y + 1), 0);
            PAL_DrawASCIICharOnSurface(*lpszText, gpScreen, PAL_XY(rect.x + 1, rect.y), 0);
         }
         PAL_DrawASCIICharOnSurface(*lpszText, gpScreen, PAL_XY(rect.x, rect.y), bColor);
         lpszText++;
         rect.x += 8;
         urect.w += 8;
      }
   }

   //
   // Update the screen area
   //
   if (fUpdate && urect.w > 0)
   {
#ifdef PAL_WIN95
      urect.w++;
      if (urect.x + urect.w > 320)
      {
         urect.w = 320 - urect.x;
      }
#endif
      VIDEO_UpdateScreen(&urect);
   }
}

VOID
PAL_DialogSetDelayTime(
   INT          iDelayTime
)
/*++
  Purpose:

    Set the delay time for dialog.

  Parameters:

    [IN]  iDelayTime - the delay time to be set.

  Return value:

    None.

--*/
{
   g_TextLib.iDelayTime = iDelayTime;
}

VOID
PAL_StartDialog(
   BYTE         bDialogLocation,
   BYTE         bFontColor,
   INT          iNumCharFace,
   BOOL         fPlayingRNG
)
/*++
  Purpose:

    Start a new dialog.

  Parameters:

    [IN]  bDialogLocation - the location of the text on the screen.

    [IN]  bFontColor - the font color of the text.

    [IN]  iNumCharFace - number of the character face in RGM.MKF.

    [IN]  fPlayingRNG - whether we are playing a RNG video or not.

  Return value:

    None.

--*/
{
   PAL_LARGE BYTE buf[16384];
   SDL_Rect       rect;

   if (gpGlobals->fInBattle && !g_fUpdatedInBattle)
   {
      //
      // Update the screen in battle, or the graphics may seem messed up
      //
      VIDEO_UpdateScreen(NULL);
      g_fUpdatedInBattle = TRUE;
   }

   g_TextLib.bIcon = 0;
   g_TextLib.posIcon = 0;
   g_TextLib.nCurrentDialogLine = 0;
   g_TextLib.posDialogTitle = PAL_XY(12, 8);
   g_TextLib.fUserSkip = FALSE;

   if (bFontColor != 0)
   {
      g_TextLib.bCurrentFontColor = bFontColor;
   }

   if (fPlayingRNG && iNumCharFace)
   {
      VIDEO_BackupScreen();
      g_TextLib.fPlayingRNG = TRUE;
   }

   switch (bDialogLocation)
   {
   case kDialogUpper:
      if (iNumCharFace > 0)
      {
         //
         // Display the character face at the upper part of the screen
         //
         if (PAL_MKFReadChunk(buf, 16384, iNumCharFace, gpGlobals->f.fpRGM) > 0)
         {
            rect.w = PAL_RLEGetWidth((LPCBITMAPRLE)buf);
            rect.h = PAL_RLEGetHeight((LPCBITMAPRLE)buf);
            rect.x = 48 - rect.w / 2;
            rect.y = 55 - rect.h / 2;

            if (rect.x < 0)
            {
               rect.x = 0;
            }

            if (rect.y < 0)
            {
               rect.y = 0;
            }

            PAL_RLEBlitToSurface((LPCBITMAPRLE)buf, gpScreen, PAL_XY(rect.x, rect.y));

            if (rect.x < 0)
            {
               rect.x = 0;
            }
            if (rect.y < 0)
            {
               rect.y = 0;
            }

            VIDEO_UpdateScreen(&rect);
         }
      }
      g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 80 : 12, 8);
      g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 96 : 44, 26);
      break;

   case kDialogCenter:
      g_TextLib.posDialogText = PAL_XY(80, 40);
      break;

   case kDialogLower:
      if (iNumCharFace > 0)
      {
         //
         // Display the character face at the lower part of the screen
         //
         if (PAL_MKFReadChunk(buf, 16384, iNumCharFace, gpGlobals->f.fpRGM) > 0)
         {
            rect.x = 270 - PAL_RLEGetWidth((LPCBITMAPRLE)buf) / 2;
            rect.y = 144 - PAL_RLEGetHeight((LPCBITMAPRLE)buf) / 2;

            PAL_RLEBlitToSurface((LPCBITMAPRLE)buf, gpScreen, PAL_XY(rect.x, rect.y));

            VIDEO_UpdateScreen(NULL);
         }
      }
      g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 4 : 12, 108);
      g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 20 : 44, 126);
      break;

   case kDialogCenterWindow:
      g_TextLib.posDialogText = PAL_XY(160, 40);
      break;
   }

   g_TextLib.bDialogPosition = bDialogLocation;
}

static VOID
PAL_DialogWaitForKey(
   VOID
)
/*++
  Purpose:

    Wait for player to press a key after showing a dialog.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_LARGE SDL_Color   palette[256];
   SDL_Color   *pCurrentPalette, t;
   int         i;

   //
   // get the current palette
   //
   pCurrentPalette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
   memcpy(palette, pCurrentPalette, sizeof(palette));

   if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
      g_TextLib.bDialogPosition != kDialogCenter)
   {
      //
      // show the icon
      //
      LPCBITMAPRLE p = PAL_SpriteGetFrame(g_TextLib.bufDialogIcons, g_TextLib.bIcon);
      if (p != NULL)
      {
         SDL_Rect rect;

         rect.x = PAL_X(g_TextLib.posIcon);
         rect.y = PAL_Y(g_TextLib.posIcon);
         rect.w = 16;
         rect.h = 16;

         PAL_RLEBlitToSurface(p, gpScreen, g_TextLib.posIcon);
         VIDEO_UpdateScreen(&rect);
      }
   }

   PAL_ClearKeyState();

   while (TRUE)
   {
      UTIL_Delay(100);

      if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
         g_TextLib.bDialogPosition != kDialogCenter)
      {
         //
         // palette shift
         //
         t = palette[0xF9];
         for (i = 0xF9; i < 0xFE; i++)
         {
            palette[i] = palette[i + 1];
         }
         palette[0xFE] = t;

         VIDEO_SetPalette(palette);
      }

      if (g_InputState.dwKeyPress != 0)
      {
         break;
      }
   }

   if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
      g_TextLib.bDialogPosition != kDialogCenter)
   {
      PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
   }

   PAL_ClearKeyState();

   g_TextLib.fUserSkip = FALSE;
}

VOID
PAL_ShowDialogText(
   LPCSTR       lpszText
)
/*++
  Purpose:

    Show one line of the dialog text.

  Parameters:

    [IN]  lpszText - the text to be shown.

  Return value:

    None.

--*/
{
   SDL_Rect        rect;
   int             x, y, len = strlen(lpszText);

   PAL_ClearKeyState();
   g_TextLib.bIcon = 0;

   if (gpGlobals->fInBattle && !g_fUpdatedInBattle)
   {
      //
      // Update the screen in battle, or the graphics may seem messed up
      //
      VIDEO_UpdateScreen(NULL);
      g_fUpdatedInBattle = TRUE;
   }

   if (g_TextLib.nCurrentDialogLine > 3)
   {
      //
      // The rest dialogs should be shown in the next page.
      //
      PAL_DialogWaitForKey();
      g_TextLib.nCurrentDialogLine = 0;
      VIDEO_RestoreScreen();
      VIDEO_UpdateScreen(NULL);
   }

   x = PAL_X(g_TextLib.posDialogText);
   y = PAL_Y(g_TextLib.posDialogText) + g_TextLib.nCurrentDialogLine * 18;

   if (g_TextLib.bDialogPosition == kDialogCenterWindow)
   {
      //
      // The text should be shown in a small window at the center of the screen
      //
#ifndef PAL_CLASSIC
      if (gpGlobals->fInBattle && g_Battle.BattleResult == kBattleResultOnGoing)
      {
         PAL_BattleUIShowText(lpszText, 1400);
      }
      else
#endif
      {
         PAL_POS    pos;
         LPBOX      lpBox;

         //
         // Create the window box
         //
         pos = PAL_XY(PAL_X(g_TextLib.posDialogText) - len * 4, PAL_Y(g_TextLib.posDialogText));
         lpBox = PAL_CreateSingleLineBox(pos, (len + 1) / 2, TRUE);

         rect.x = PAL_X(pos);
         rect.y = PAL_Y(pos);
         rect.w = 320 - rect.x * 2 + 32;
         rect.h = 64;

         //
         // Show the text on the screen
         //
         pos = PAL_XY(PAL_X(pos) + 8 + ((len & 1) << 2), PAL_Y(pos) + 10);
         PAL_DrawText(lpszText, pos, 0, FALSE, FALSE);
         VIDEO_UpdateScreen(&rect);

         PAL_DialogWaitForKey();

         //
         // Delete the box
         //
         PAL_DeleteBox(lpBox);
         VIDEO_UpdateScreen(&rect);

         PAL_EndDialog();
      }
   }
   else
   {
      if (g_TextLib.nCurrentDialogLine == 0 &&
         g_TextLib.bDialogPosition != kDialogCenter &&
         (((BYTE)lpszText[len - 1] == 0x47 && (BYTE)lpszText[len - 2] == 0xA1) ||
          ((BYTE)lpszText[len - 1] == 0xBA && (BYTE)lpszText[len - 2] == 0xA3) ||
          ((BYTE)lpszText[len - 1] == 0xC3 && (BYTE)lpszText[len - 2] == 0xA1) ||
          ((BYTE)lpszText[len - 1] == ':')))
      {
         //
         // name of character
         //
         PAL_DrawText(lpszText, g_TextLib.posDialogTitle, FONT_COLOR_CYAN_ALT, TRUE, TRUE);
      }
      else
      {
         //
         // normal texts
         //
         char text[3];

         if (!g_TextLib.fPlayingRNG && g_TextLib.nCurrentDialogLine == 0)
         {
            //
            // Save the screen before we show the first line of dialog
            //
            VIDEO_BackupScreen();
         }

         while (lpszText != NULL && *lpszText != '\0')
         {
            switch (*lpszText)
            {
            case '-':
               //
               // Set the font color to Cyan
               //
               if (g_TextLib.bCurrentFontColor == FONT_COLOR_CYAN)
               {
                  g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
               }
               else
               {
                  g_TextLib.bCurrentFontColor = FONT_COLOR_CYAN;
               }
               lpszText++;
               break;
#ifndef PAL_WIN95
            case '\'':
               //
               // Set the font color to Red
               //
               if (g_TextLib.bCurrentFontColor == FONT_COLOR_RED)
               {
                  g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
               }
               else
               {
                  g_TextLib.bCurrentFontColor = FONT_COLOR_RED;
               }
               lpszText++;
               break;
#endif
            case '\"':
               //
               // Set the font color to Yellow
               //
               if (g_TextLib.bCurrentFontColor == FONT_COLOR_YELLOW)
               {
                  g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
               }
               else
               {
                  g_TextLib.bCurrentFontColor = FONT_COLOR_YELLOW;
               }
               lpszText++;
               break;

            case '$':
               //
               // Set the delay time of text-displaying
               //
               g_TextLib.iDelayTime = atoi(lpszText + 1) * 10 / 7;
               lpszText += 3;
               break;

            case '~':
               //
               // Delay for a period and quit
               //
               UTIL_Delay(atoi(lpszText + 1) * 80 / 7);
               g_TextLib.nCurrentDialogLine = 0;
               g_TextLib.fUserSkip = FALSE;
               return; // don't go further

            case ')':
               //
               // Set the waiting icon
               //
               g_TextLib.bIcon = 1;
               lpszText++;
               break;

            case '(':
               //
               // Set the waiting icon
               //
               g_TextLib.bIcon = 2;
               lpszText++;
               break;

            case '\\':
               lpszText++;

            default:
               if (*lpszText & 0x80)
               {
                  text[0] = lpszText[0];
                  text[1] = lpszText[1];
                  text[2] = '\0';
                  lpszText += 2;
               }
               else
               {
                  text[0] = *lpszText;
                  text[1] = '\0';
                  lpszText++;
               }

               PAL_DrawText(text, PAL_XY(x, y), g_TextLib.bCurrentFontColor, TRUE, TRUE);
               x += ((text[0] & 0x80) ? 16 : 8);

               if (!g_TextLib.fUserSkip)
               {
                  PAL_ClearKeyState();
                  UTIL_Delay(g_TextLib.iDelayTime * 8);

                  if (g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
                  {
                     //
                     // User pressed a key to skip the dialog
                     //
                     g_TextLib.fUserSkip = TRUE;
                  }
               }
            }
         }

         g_TextLib.posIcon = PAL_XY(x, y);
         g_TextLib.nCurrentDialogLine++;
      }
   }
}

VOID
PAL_ClearDialog(
   BOOL       fWaitForKey
)
/*++
  Purpose:

    Clear the state of the dialog.

  Parameters:

    [IN]  fWaitForKey - whether wait for any key or not.

  Return value:

    None.

--*/
{
   if (g_TextLib.nCurrentDialogLine > 0 && fWaitForKey)
   {
      PAL_DialogWaitForKey();
   }

   g_TextLib.nCurrentDialogLine = 0;

   if (g_TextLib.bDialogPosition == kDialogCenter)
   {
      g_TextLib.posDialogTitle = PAL_XY(12, 8);
      g_TextLib.posDialogText = PAL_XY(44, 26);
      g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
      g_TextLib.bDialogPosition = kDialogUpper;
   }
}

VOID
PAL_EndDialog(
   VOID
)
/*++
  Purpose:

    Ends a dialog.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_ClearDialog(TRUE);

   //
   // Set some default parameters, as there are some parts of script
   // which doesn't have a "start dialog" instruction before showing the dialog.
   //
   g_TextLib.posDialogTitle = PAL_XY(12, 8);
   g_TextLib.posDialogText = PAL_XY(44, 26);
   g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
   g_TextLib.bDialogPosition = kDialogUpper;
   g_TextLib.fUserSkip = FALSE;
   g_TextLib.fPlayingRNG = FALSE;
}

BOOL
PAL_IsInDialog(
   VOID
)
/*++
  Purpose:

    Check if there are dialog texts on the screen.

  Parameters:

    None.

  Return value:

    TRUE if there are dialog texts on the screen, FALSE if not.

--*/
{
   return (g_TextLib.nCurrentDialogLine != 0);
}

BOOL
PAL_DialogIsPlayingRNG(
   VOID
)
/*++
  Purpose:

    Check if the script used the RNG playing parameter when displaying texts.

  Parameters:

    None.

  Return value:

    TRUE if the script used the RNG playing parameter, FALSE if not.

--*/
{
   return g_TextLib.fPlayingRNG;
}
