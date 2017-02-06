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
// Modified by Lou Yihua <louyihua@21cn.com> with Unicode support, 2015
//

#include "font.h"
#include "util.h"
#include "text.h"

#define _FONT_C

#include "fontglyph.h"
#include "ascii.h"

static int _font_height = 16;

uint8_t reverseBits(uint8_t x) {
    uint8_t y = 0;
    for(int i = 0 ; i < 8; i++){
        y <<= 1;
        y |= (x & 1);
        x >>= 1;
    }
    return y;
}
INT
PAL_InitEmbeddedFont(
   VOID
)
/*++
  Purpose:

    None.

  Parameters:

    None.

  Return value:

    Always return 0.

--*/
{
	FILE *fp;
	char *char_buf;
	wchar_t *wchar_buf;
	int nBytes, nChars, i;

	//
	// Load the wor16.asc file.
	//
	if (NULL == (fp = UTIL_OpenFile("wor16.asc")))
	{
		return 0;
	}

	//
	// Get the size of wor16.asc file.
	//
	fseek(fp, 0, SEEK_END);
	nBytes = ftell(fp);

	//
	// Allocate buffer & read all the character codes.
	//
	if (NULL == (char_buf = (char *)malloc(nBytes)))
	{
		fclose(fp);
		return 0;
	}
	fseek(fp, 0, SEEK_SET);
	fread(char_buf, 1, nBytes, fp);

	//
	// Close wor16.asc file.
	//
	fclose(fp);

	//
	// Convert characters into unicode
	//
	nChars = PAL_MultiByteToWideChar(char_buf, nBytes, NULL, 0);
	if (NULL == (wchar_buf = (wchar_t *)malloc(nChars * sizeof(wchar_t))))
	{
		free(char_buf);
		return 0;
	}
	PAL_MultiByteToWideChar(char_buf, nBytes, wchar_buf, nChars);
	free(char_buf);

	//
	// Read bitmaps from wor16.fon file.
	//
	fp = UTIL_OpenFile("wor16.fon");

	//
	// The font glyph data begins at offset 0x682 in wor16.fon.
	//
	fseek(fp, 0x682, SEEK_SET);

	//
	// Replace the original fonts
	//
	for (i = 0; i < nChars; i++)
	{
		wchar_t w = (wchar_buf[i] >= unicode_upper_base) ? (wchar_buf[i] - unicode_upper_base + 0xd800) : wchar_buf[i];
		fread(unicode_font[w], 30, 1, fp);
		unicode_font[w][30] = 0;
		unicode_font[w][31] = 0;
	}
	free(wchar_buf);

	fclose(fp);

        assert(reverseBits(0x63)==0xc6);
        assert(reverseBits(0x7f)==0xfe);
        assert(reverseBits(0x03)==0xc0);
        assert(reverseBits(0x3f)==0xfc);
		for (i = 0; i < 0x80; i++)
		{
            int j=-1;while(j++<16)unicode_font[i][j]=reverseBits(iso_font[i*15+j]);
			unicode_font[i][15] = 0;
		}
	_font_height = 15;

	return 0;
}

INT
PAL_LoadBdfFont(
   LPCSTR      pszBdfFileName
)
/*++
  Purpose:

    Loads a BDF bitmap font file.

  Parameters:

    [IN]  pszBdfFileName - Name of BDF bitmap font file..

  Return value:

    0 = success, -1 = failure.

--*/
{
   char buf[4096];
   int state = 0;
   int codepage = -1;

   DWORD dwEncoding = 0;
   BYTE bFontGlyph[32] = {0};
   int iCurHeight = 0;

   FILE *fp = UTIL_OpenFileForMode(pszBdfFileName, "r");

   if (fp == NULL)
   {
      return -1;
   }

   while (fgets(buf, 4096, fp) != NULL)
   {
      if (state == 0)
      {
         if (strncmp(buf, "CHARSET_REGISTRY", 16) == 0)
         {
            if (strstr(buf, "Big5") != NULL)
            {
               codepage = CP_BIG5;
            }
            else if (strstr(buf, "BIG5") != NULL)
            {
               codepage = CP_BIG5;
            }
            //else if (strstr(buf, "JISX0208") != NULL)
            //
            //  codepage = CP_JISX0208;
            //}
         }
         else if (strncmp(buf, "ENCODING", 8) == 0)
         {
            dwEncoding = atoi(buf + 8);
         }
         else if (strncmp(buf, "BITMAP", 6) == 0)
         {
            state = 1;
            iCurHeight = 0;
            memset(bFontGlyph, 0, sizeof(bFontGlyph));
         }
      }
      else if (state == 1)
      {
         if (strncmp(buf, "ENDCHAR", 7) == 0)
         {
            //
            // Replace the original fonts
            //
            BYTE szCp[3];
            szCp[0] = (dwEncoding >> 8) & 0xFF;
            szCp[1] = dwEncoding & 0xFF;
            szCp[2] = 0;
            wchar_t wc[2] = { 0 };
            PAL_MultiByteToWideCharCP(codepage, (LPCSTR)szCp, 2, wc, 1);
            if (wc[0] != 0)
            {
               wchar_t w = (wc[0] >= unicode_upper_base) ? (wc[0] - unicode_upper_base + 0xd800) : wc[0];
               memcpy(unicode_font[w], bFontGlyph, sizeof(bFontGlyph));
            }

            state = 0;
         }
         else
         {
            if (iCurHeight < 16)
            {
               WORD wCode = strtoul(buf, NULL, 16);
               bFontGlyph[iCurHeight * 2] = (wCode >> 8);
               bFontGlyph[iCurHeight * 2 + 1] = (wCode & 0xFF);
               iCurHeight++;
            }
         }
      }
   }

   _font_height = 16;
   fclose(fp);
   return 0;
}

VOID
PAL_FreeFont(
   VOID
)
/*++
  Purpose:

    None.

  Parameters:

    None.

  Return value:

    None.

--*/
{
}

VOID
PAL_DrawCharOnSurface(
   WORD                     wChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor,
   BOOL                     fUse8x8Font
)
/*++
  Purpose:

    Draw a Unicode character on a surface.

  Parameters:

    [IN]  wChar - the unicode character to be drawn.

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
{
	int       i, j;
	int       x = PAL_X(pos), y = PAL_Y(pos);

	//
	// Check for NULL pointer & invalid char code.
	//
	if (lpSurface == NULL || (wChar >= 0xd800 && wChar < unicode_upper_base) ||
		wChar >= unicode_upper_top || (_font_height == 8 && wChar >= 0x100))
	{
		return;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - 0xd800);
	}

	//
	// Draw the character to the surface.
	//
	LPBYTE dest = (LPBYTE)lpSurface->pixels + y * lpSurface->pitch + x;
	LPBYTE top = (LPBYTE)lpSurface->pixels + lpSurface->h * lpSurface->pitch;
	if (fUse8x8Font)
	{
		for (i = 0; i < 8 && dest < top; i++, dest += lpSurface->pitch)
		{
			for (j = 0; j < 8 && x + j < lpSurface->w; j++)
			{
				if (iso_font_8x8[wChar][i] & (1 << j))
				{
					dest[j] = bColor;
				}
			}
		}
	}
	else
	{
		if (font_width[wChar] == 32)
		{
			for (i = 0; i < _font_height * 2 && dest < top; i += 2, dest += lpSurface->pitch)
			{
				for (j = 0; j < 8 && x + j < lpSurface->w; j++)
				{
					if (unicode_font[wChar][i] & (1 << (7 - j)))
					{
						dest[j] = bColor;
					}
				}
				for (j = 0; j < 8 && x + j + 8 < lpSurface->w; j++)
				{
					if (unicode_font[wChar][i + 1] & (1 << (7 - j)))
					{
						dest[j + 8] = bColor;
					}
				}
			}
		}
		else
		{
			for (i = 0; i < _font_height && dest < top; i++, dest += lpSurface->pitch)
			{
				for (j = 0; j < 8 && x + j < lpSurface->w; j++)
				{
					if (unicode_font[wChar][i] & (1 << (7 - j)))
					{
						dest[j] = bColor;
					}
				}
			}
		}
	}
}

INT
PAL_CharWidth(
   WORD                     wChar
)
/*++
  Purpose:

    Get the text width of a character.

  Parameters:

    [IN]  wChar - the unicode character for width calculation.

  Return value:

    The width of the character, 16 for full-width char and 8 for half-width char.

--*/
{
	if ((wChar >= 0xd800 && wChar < unicode_upper_base) || wChar >= unicode_upper_top)
	{
		return 0;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - 0xd800);
	}

	return font_width[wChar] >> 1;
}

INT
PAL_FontHeight(
   VOID
)
{
	return _font_height;
}
