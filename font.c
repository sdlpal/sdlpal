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

#include "font.h"
#include "util.h"
#include "text.h"

#define _FONT_C

#include "fontglyph.h"
#include "fontglyph_cn.h"
#include "fontglyph_tw.h"
#include "fontglyph_jp.h"
#include "ascii.h"

static int _font_height = 16;

static uint8_t reverseBits(uint8_t x) {
    uint8_t y = 0;
    for (int i = 0 ; i < 8; i++){
        y <<= 1;
        y |= (x & 1);
        x >>= 1;
    }
    return y;
}

#ifdef DEBUG
void dump_font(BYTE *buf,int rows, int cols)
{
   for(int row=0;row<rows;row++)
   {
      for( int bit=0;bit<cols;bit++)
         if( buf[row] & (uint8_t)pow(2,bit) )
            printf("*");
         else
            printf(" ");
      printf("\n");
   }
}

static uint16_t reverseBits16(uint16_t x)
{
   //specified to unifont structure; not common means
   uint8_t l = reverseBits(x);
   uint8_t h = reverseBits(x >> 8);
   return h << 8 | l;
}

void dump_font16(WORD *buf,int rows, int cols)
{
   for(int row=0;row<rows;row++)
   {
      for( int bit=0;bit<cols;bit++)
         if( reverseBits16(buf[row]) & (uint16_t)pow(2,bit) )
            printf("*");
         else
            printf(" ");
      printf("\n");
   }
}
#endif

static void PAL_LoadISOFont(void)
{
    int         i, j;

    for (i = 0; i < sizeof(iso_font) / 15; i++)
    {
        for (j = 0; j < 15; j++)
        {
            unicode_font[i][j] = reverseBits(iso_font[i * 15 + j]);
        }

        unicode_font[i][15] = 0;
        font_width[i] = 16;
    }
}

static void PAL_LoadEmbeddedFont(void)
{
	FILE *fp;
	char *char_buf;
	wchar_t *wchar_buf;
	size_t nBytes;
	int nChars, i;

	//
	// Load the wor16.asc file.
	//
	if (NULL == (fp = UTIL_OpenFile("wor16.asc")))
	{
		return;
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
		return;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(char_buf, 1, nBytes, fp) < nBytes)
	{
		fclose(fp);
		return;
	}

	//
	// Close wor16.asc file.
	//
	fclose(fp);

	//
	// Detect the codepage of 'wor16.asc' and exit if not BIG5 or probability < 99
	// Note: 100% probability is impossible as the function does not recognize some special
	// characters such as bopomofo that may be used by 'wor16.asc'.
	//
	if (PAL_DetectCodePageForString(char_buf, nBytes, CP_BIG5, &i) != CP_BIG5 || i < 99)
	{
		free(char_buf);
		return;
	}

	//
	// Convert characters into unicode
	// Explictly specify BIG5 here for compatibility with codepage auto-detection
	//
	nChars = PAL_MultiByteToWideCharCP(CP_BIG5, char_buf, nBytes, NULL, 0);
	if (NULL == (wchar_buf = (wchar_t *)malloc(nChars * sizeof(wchar_t))))
	{
		free(char_buf);
		return;
	}
	PAL_MultiByteToWideCharCP(CP_BIG5, char_buf, nBytes, wchar_buf, nChars);
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
		wchar_t w = (wchar_buf[i] >= unicode_upper_base) ? (wchar_buf[i] - unicode_upper_base + unicode_lower_top) : wchar_buf[i];
		if (fread(unicode_font[w], 30, 1, fp) == 1)
		{
			unicode_font[w][30] = 0;
			unicode_font[w][31] = 0;
		}
		font_width[w] = 32;
	}
	free(wchar_buf);

	fclose(fp);

	_font_height = 15;
}

INT
PAL_LoadUserFont(
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
   int state = 0, fstate = 0;
   int codepage = -1;
   DWORD dwEncoding = 0;
   BYTE bFontGlyph[32] = {0};
   int iCurHeight = 0;
   int bbw = 0, bbh = 0, bbox, bboy;

   FILE *fp = UTIL_OpenFileForMode(pszBdfFileName, "r");

   if (fp == NULL)
   {
      return -1;
   }

   _font_height = 16;

   while (fgets(buf, 4096, fp) != NULL)
   {
      if (state == 0)
      {
         if (strncmp(buf, "CHARSET_REGISTRY", 16) == 0)
         {
            if (strcasestr(buf, "Big5") != NULL)
            {
               codepage = CP_BIG5;
            }
            else if (strcasestr(buf, "GBK") != NULL || strcasestr(buf, "GB2312") != NULL)
            {
               codepage = CP_GBK;
            }
            else if (strcasestr(buf, "ISO10646") != NULL)
            {
               codepage = CP_UCS;
            }
            //else if (strstr(buf, "JISX0208") != NULL)
            //
            //  codepage = CP_JISX0208;
            //}
            else
            {
               TerminateOnError("Charset of %s is %s, which is not a supported yet.",pszBdfFileName,buf);
            }
         }
         else if (strncmp(buf, "ENCODING", 8) == 0)
         {
            dwEncoding = atoi(buf + 8);
         }
         else if (strncmp(buf, "SIZE", 3) == 0)
         {
            int bytes_consumed = 0, bytes_now;
            int got_size;
            BOOL got_expected = FALSE;
            char size[10];
            sscanf(buf + bytes_consumed, "%s%n", size, &bytes_now);
            bytes_consumed += bytes_now;
            while (sscanf(buf + bytes_consumed, "%d%n", &got_size, &bytes_now))
            {
               bytes_consumed += bytes_now;
               if (got_size == 16 || got_size == 15)
               {
                  _font_height = got_size;
                  got_expected = TRUE;
               }
            }
            if (!got_expected)
               TerminateOnError("%s not contains expected font size 15/16!", pszBdfFileName);
         }
         else if (strncmp(buf, "BBX", 3) == 0)
         {
            int bytes_consumed = 0, bytes_now;
            char bbx[10];
            sscanf(buf+bytes_consumed,"%s%n",bbx,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbw,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbh,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbox,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bboy,&bytes_now);bytes_consumed += bytes_now;
         }
         else if (strncmp(buf, "BITMAP", 6) == 0)
         {
            state = 1;
            iCurHeight = 0;
            for(iCurHeight=0;iCurHeight<bboy;iCurHeight++){
               bFontGlyph[iCurHeight * 2] = 0;
               bFontGlyph[iCurHeight * 2 + 1] = 0;
            }
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

            if (codepage == CP_GBK && dwEncoding > 0xFF)
            {
               szCp[0] |= 0x80;
               szCp[1] |= 0x80;
            }

            wchar_t wc[2] = { 0 };
            PAL_MultiByteToWideCharCP(codepage, (LPCSTR)szCp, 2, wc, 1);
            if (wc[0] != 0)
            {
               wchar_t w = (wc[0] >= unicode_upper_base) ? (wc[0] - unicode_upper_base + unicode_lower_top) : wc[0];
               if (w < sizeof(unicode_font) / sizeof(unicode_font[0]))
               {
                  memcpy(unicode_font[w], bFontGlyph, sizeof(bFontGlyph));
               }
            }

            state = 0;
         }
         else
         {
            if (iCurHeight < bbh )
            {
               WORD wCode = strtoul(buf, NULL, 16);
               if(bbw <= 8)
               {
                  switch(fstate)
                  {
                     case 0:
                        bFontGlyph[iCurHeight * 2] = wCode;
                        fstate = 1;
                        break;
                     case 1:
                        bFontGlyph[iCurHeight * 2+1] = wCode;
                        fstate = 0;
                        iCurHeight++;
                        break;
                  }
               }else{
                  bFontGlyph[iCurHeight * 2] = (wCode >> 8);
                  bFontGlyph[iCurHeight * 2 + 1] = (wCode & 0xFF);
                  iCurHeight++;
               }
            }
         }
      }
   }

   fclose(fp);
   return 0;
}

int
PAL_InitFont(
   const CONFIGURATION* cfg
)
{
#define PAL_LOAD_INTERNAL_FONT(fontdata, height) \
   { \
      for (int i = 0; i < sizeof(fontdata) / sizeof(fontdata[0]); i++) \
      { \
         wchar_t w = fontdata[i].code; \
         w = (w >= unicode_upper_base) ? (w - unicode_upper_base + unicode_lower_top) : w; \
         if (w < sizeof(unicode_font) / sizeof(unicode_font[0])) \
         { \
            memcpy(unicode_font[w], fontdata[i].data, 32); \
            font_width[w] = 32; \
         } \
      } \
      _font_height = height; \
   }

   if (!cfg->pszMsgFile)
   {
      PAL_LoadEmbeddedFont();
   }

   if (g_TextLib.fUseISOFont)
   {
      PAL_LoadISOFont();
   }

   if (cfg->pszFontFile)
   {
      PAL_LoadUserFont(cfg->pszFontFile);
   }
   else if (_font_height != 15)
   {
      switch (g_TextLib.iFontFlavor)
      {
      case kFontFlavorAuto:
         switch (PAL_GetCodePage())
         {
         case CP_GBK:
            PAL_LOAD_INTERNAL_FONT(fontglyph_cn, 16);
            break;

         case CP_BIG5:
            PAL_LOAD_INTERNAL_FONT(fontglyph_tw, 15);
            break;

         default:
            break;
         }
         break;

      case kFontFlavorSimpChin:
         PAL_LOAD_INTERNAL_FONT(fontglyph_cn, 16);
         break;

      case kFontFlavorTradChin:
         PAL_LOAD_INTERNAL_FONT(fontglyph_tw, 15);
         break;

      case kFontFlavorJapanese:
         PAL_LOAD_INTERNAL_FONT(fontglyph_jp, 16);
         break;

      case kFontFlavorUnifont:
      default:
         break;
      }
   }

   return 0;
}

void
PAL_FreeFont(
	void
)
{
}

void
PAL_DrawCharOnSurface(
	uint16_t                 wChar,
	SDL_Surface             *lpSurface,
	PAL_POS                  pos,
	uint8_t                  bColor,
	BOOL                     fUse8x8Font
)
{
	int       i, j;
	int       x = PAL_X(pos), y = PAL_Y(pos);

	//
	// Check for NULL pointer & invalid char code.
	//
	if (lpSurface == NULL || (wChar >= unicode_lower_top && wChar < unicode_upper_base) ||
		wChar >= unicode_upper_top || (_font_height == 8 && wChar >= 0x100))
	{
		return;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - unicode_lower_top);
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

int
PAL_CharWidth(
	uint16_t                 wChar
)
{
	if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || wChar >= unicode_upper_top)
	{
		return 0;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - unicode_lower_top);
	}

	return font_width[wChar] >> 1;
}

int
PAL_FontHeight(
	void
)
{
	return _font_height;
}
