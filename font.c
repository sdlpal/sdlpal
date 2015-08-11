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

INT
PAL_InitFont(
   BOOL      fUseEmbeddedFonts
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
	if (fUseEmbeddedFonts)
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

		for (i = 0; i < 0x80; i++)
		{
			memcpy(unicode_font[i], &iso_font[i * 15], 15);
			unicode_font[i][15] = 0;
		}
		_font_height = 15;
	}

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
   BYTE                     bColor
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
	if (lpSurface == NULL || (wChar >= 0xd800 && wChar < unicode_upper_base) || wChar >= unicode_upper_top)
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
