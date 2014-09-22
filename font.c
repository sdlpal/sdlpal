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

#include "font.h"
#include "ascii.h"
#include "util.h"

#ifdef PAL_WIN95

/*
 * Portions based on:
 *
 * YH - Console Chinese Environment -
 * Copyright (C) 1999 Red Flag Linux (office@sonata.iscas.ac.cn)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY RED FLAG LINUX ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Portions based on:
 *
 * KON2 - Kanji ON Console -
 * Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
 *
 * CCE - Console Chinese Environment -
 * Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY TAKASHI MANABE ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 */

#include "gbfont.h"
#include "big5font.h"

BOOL fIsBig5 = FALSE;

INT
PAL_InitFont(
   VOID
)
/*++
  Purpose:

    Load the font files.

  Parameters:

    None.

  Return value:

    0 if succeed, -1 if cannot allocate memory, -2 if cannot load files.

--*/
{
   FILE *fp;

   fp = fopen(PAL_PREFIX "word.dat", "rb");
   if (!fp)
   {
      return 0;
   }

   fseek(fp, 0x1E, SEEK_SET);
   if (fgetc(fp) == 0xAA)
   {
      fIsBig5 = TRUE;
   }

   fclose(fp);
   return 0;
}

VOID
PAL_FreeFont(
   VOID
)
/*++
  Purpose:

    Free the memory used for fonts.

  Parameters:

    None.

  Return value:

    None.

--*/
{
}

static BOOL is_gb(unsigned char b1, unsigned char b2)
{
   if (b1 < 0xa1 || b1 > 0xfe)
      return FALSE;
   if (b2 < 0xa1 || b2 > 0xfe)
      return FALSE;
   return TRUE;
}

VOID
PAL_DrawCharOnSurface(
   WORD                     wChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor
)
/*++
  Purpose:

    Draw a BIG-5 Chinese character on a surface.

  Parameters:

    [IN]  wChar - the character to be drawn (in GB2312/BIG5).

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
{
   int       i, j, dx;
   int       x = PAL_X(pos), y = PAL_Y(pos);
   LPBYTE    pChar;
   BYTE      ch1, ch2;

   //
   // Check for NULL pointer.
   //
   if (lpSurface == NULL)
   {
      return;
   }

   //
   // Locate for this character in the font lib.
   //
   ch1 = wChar & 0xff;
   ch2 = wChar >> 8;

   if (fIsBig5)
   {
      if (ch2 < 0xa1)
         pChar = &big5font[((ch1 - 0xA1) * 157 + ch2 - 0x40) << 5] + 8;
      else
         pChar = &big5font[((ch1 - 0xA1) * 157 + 63 + ch2 - 0xA1) << 5] + 8;
   }
   else
   {
      if (!is_gb(ch1, ch2))
      {
         return;
      }
      pChar = &gbfont[((ch1 - 0xa1) * 94 + (ch2 - 0xa1)) * 32];
   }

   if (pChar == NULL) return;

   //
   // Draw the character to the surface.
   //
   if (y >= lpSurface->h) return;

   y *= lpSurface->pitch;
   for (i = 0; i < 32; i++)
   {
      dx = x + ((i & 1) << 3);
      for (j = 0; j < 8; j++)
      {
         if (pChar[i] & (1 << (7 - j)))
         {
            if (dx < lpSurface->w)
            {
               ((LPBYTE)(lpSurface->pixels))[y + dx] = bColor;
            }
            else
            {
               break;
            }
         }
         dx++;
      }
      y += (i & 1) * lpSurface->pitch;
      if (y / lpSurface->pitch >= lpSurface->h)
      {
         break;
      }
   }
}

VOID
PAL_DrawASCIICharOnSurface(
   BYTE                     bChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor
)
/*++
  Purpose:

    Draw a ASCII character on a surface.

  Parameters:

    [IN]  bChar - the character to be drawn.

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
{
   int i, j, dx;
   int x = PAL_X(pos), y = PAL_Y(pos);
   LPBYTE pChar;

   pChar = &iso_font[(int)(bChar & ~128) * 15];

   //
   // Check for NULL pointer.
   //
   if (lpSurface == NULL)
   {
      return;
   }

   //
   // Draw the character to the surface.
   //
   if (y >= lpSurface->h) return;

   y *= lpSurface->pitch;
   for (i = 0; i < 15; i++)
   {
      dx = x;
      for (j = 0; j < 8; j++)
      {
         if (pChar[i] & (1 << j))
         {
            if (dx < lpSurface->w)
            {
               ((LPBYTE)(lpSurface->pixels))[y + dx] = bColor;
            }
            else
            {
               break;
            }
         }
         dx++;
      }
      y += lpSurface->pitch;
      if (y / lpSurface->pitch >= lpSurface->h)
      {
         break;
      }
   }
}

#else

typedef struct tagFont
{
   LPWORD           lpBufChar;
   LPBYTE           lpBufGlyph;
   INT              nChar;
} FONT, *LPFONT;

static LPFONT gpFont = NULL;

INT
PAL_InitFont(
   VOID
)
/*++
  Purpose:

    Load the font files.

  Parameters:

    None.

  Return value:

    0 if succeed, -1 if cannot allocate memory, -2 if cannot load files.

--*/
{
   FILE *fp;

   if (gpFont != NULL)
   {
      //
      // Already initialized
      //
      return 0;
   }

   gpFont = (LPFONT)calloc(1, sizeof(FONT));
   if (gpFont == NULL)
   {
      return -1;
   }

   //
   // Load the wor16.asc file.
   //
   fp = UTIL_OpenRequiredFile("wor16.asc");

   //
   // Get the size of wor16.asc file.
   //
   fseek(fp, 0, SEEK_END);
   gpFont->nChar = ftell(fp);
   gpFont->nChar /= 2;

   //
   // Read all the character codes.
   //
   gpFont->lpBufChar = (LPWORD)calloc(gpFont->nChar, sizeof(WORD));
   if (gpFont->lpBufChar == NULL)
   {
      free(gpFont);
      gpFont = NULL;
      return -1;
   }

   fseek(fp, 0, SEEK_SET);
   fread(gpFont->lpBufChar, sizeof(WORD), gpFont->nChar, fp);

   //
   // Close wor16.asc file.
   //
   fclose(fp);

   //
   // Read all bitmaps from wor16.fon file.
   //
   fp = UTIL_OpenRequiredFile("wor16.fon");

   gpFont->lpBufGlyph = (LPBYTE)calloc(gpFont->nChar, 30);
   if (gpFont->lpBufGlyph == NULL)
   {
      free(gpFont->lpBufChar);
      free(gpFont);
      gpFont = NULL;
      return -1;
   }

   //
   // The font glyph data begins at offset 0x682 in wor16.fon.
   //
   fseek(fp, 0x682, SEEK_SET);
   fread(gpFont->lpBufGlyph, 30, gpFont->nChar, fp);
   fclose(fp);

   return 0;
}

VOID
PAL_FreeFont(
   VOID
)
/*++
  Purpose:

    Free the memory used for fonts.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gpFont != NULL)
   {
      free(gpFont->lpBufChar);
      free(gpFont->lpBufGlyph);
      free(gpFont);
   }

   gpFont = NULL;
}

VOID
PAL_DrawCharOnSurface(
   WORD                     wChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor
)
/*++
  Purpose:

    Draw a BIG-5 Chinese character on a surface.

  Parameters:

    [IN]  wChar - the character to be drawn (in BIG-5).

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
{
   int       i, j, dx;
   int       x = PAL_X(pos), y = PAL_Y(pos);
   LPBYTE    pChar;

   //
   // Check for NULL pointer.
   //
   if (lpSurface == NULL || gpFont == NULL)
   {
      return;
   }

   //
   // Locate for this character in the font lib.
   //
   for (i = 0; i < gpFont->nChar; i++)
   {
      if (gpFont->lpBufChar[i] == wChar)
      {
         break;
      }
   }

   if (i >= gpFont->nChar)
   {
      //
      // This character does not exist in the font lib.
      //
      return;
   }

   pChar = gpFont->lpBufGlyph + i * 30;

   //
   // Draw the character to the surface.
   //
   y *= lpSurface->pitch;
   for (i = 0; i < 30; i++)
   {
      dx = x + ((i & 1) << 3);
      for (j = 0; j < 8; j++)
      {
         if (pChar[i] & (1 << (7 - j)))
         {
            ((LPBYTE)(lpSurface->pixels))[y + dx] = bColor;
         }
         dx++;
      }
      y += (i & 1) * lpSurface->pitch;
   }
}

VOID
PAL_DrawASCIICharOnSurface(
   BYTE                     bChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor
)
/*++
  Purpose:

    Draw a ASCII character on a surface.

  Parameters:

    [IN]  bChar - the character to be drawn.

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
{
   int i, j, dx;
   int x = PAL_X(pos), y = PAL_Y(pos);
   LPBYTE pChar = &iso_font[(int)(bChar & ~128) * 15];

   //
   // Check for NULL pointer.
   //
   if (lpSurface == NULL)
   {
      return;
   }

   //
   // Draw the character to the surface.
   //
   y *= lpSurface->pitch;
   for (i = 0; i < 15; i++)
   {
      dx = x;
      for (j = 0; j < 8; j++)
      {
         if (pChar[i] & (1 << j))
         {
            ((LPBYTE)(lpSurface->pixels))[y + dx] = bColor;
         }
         dx++;
      }
      y += lpSurface->pitch;
   }
}

#endif
