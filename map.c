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

#include "palcommon.h"
#include "map.h"

LPPALMAP
PAL_LoadMap(
   INT               iMapNum,
   FILE             *fpMapMKF,
   FILE             *fpGopMKF
)
/*++
  Purpose:

    Load the specified map from the MKF file, as well as the tile bitmaps.

  Parameters:

    [IN]  iMapNum - Number of the map to load.

    [IN]  fpMapMKF - Pointer to the fopen'ed map.mkf file, which
                     contains the map tile data.

    [IN]  fpGopMKF - Pointer to the fopen'ed gop.mkf file, which
                     contains the tile bitmaps. The bitmap can be read
                     by PAL_SpriteGetFrame() function.

  Return value:

    Pointer to the loaded map. NULL if failed.

--*/
{
   LPBYTE                     buf;
   INT                        size, i, j;
   LPPALMAP                   map;

   //
   // Check for invalid map number.
   //
   if (iMapNum >= PAL_MKFGetChunkCount(fpMapMKF) ||
      iMapNum >= PAL_MKFGetChunkCount(fpGopMKF) ||
      iMapNum <= 0)
   {
      return NULL;
   }

   //
   // Load the map tile data.
   //
   size = PAL_MKFGetChunkSize(iMapNum, fpMapMKF);

   //
   // Allocate a temporary buffer for the compressed data.
   //
   buf = (LPBYTE)malloc(size);
   if (buf == NULL)
   {
      return NULL;
   }

   //
   // Create the map instance.
   //
   map = (LPPALMAP)malloc(sizeof(PALMAP));
   if (map == NULL)
   {
      return NULL;
   }

   //
   // Read the map data.
   //
   if (PAL_MKFReadChunk(buf, size, iMapNum, fpMapMKF) < 0)
   {
      free(buf);
      free(map);
      return NULL;
   }

   //
   // Decompress the tile data.
   //
   if (Decompress(buf, (LPBYTE)(map->Tiles), sizeof(map->Tiles)) < 0)
   {
      free(map);
      free(buf);
      return NULL;
   }

   //
   // The compressed data is useless now; delete it.
   //
   free(buf);

   //
   // Adjust the endianness of the decompressed data.
   //
   for (i = 0; i < 128; i++)
   {
      for (j = 0; j < 64; j++)
      {
         map->Tiles[i][j][0] = SWAP32(map->Tiles[i][j][0]);
         map->Tiles[i][j][1] = SWAP32(map->Tiles[i][j][1]);
      }
   }

   //
   // Load the tile bitmaps.
   //
   size = PAL_MKFGetChunkSize(iMapNum, fpGopMKF);
   if (size <= 0)
   {
      free(map);
      return NULL;
   }
   map->pTileSprite = (LPSPRITE)malloc(size);
   if (map->pTileSprite == NULL)
   {
      free(map);
      return NULL;
   }
   if (PAL_MKFReadChunk(map->pTileSprite, size, iMapNum, fpGopMKF) < 0)
   {
      free(map);
      return NULL;
   }

   //
   // Done.
   //
   map->iMapNum = iMapNum;

   return map;
}

VOID
PAL_FreeMap(
   LPPALMAP          lpMap
)
/*++
  Purpose:

    Free a loaded map, as well as the tile bitmaps.

  Parameters:

    [IN]  lpMap - Pointer to the loaded map structure.

  Return value:

    None.

--*/
{
   //
   // Check for NULL pointer.
   //
   if (lpMap == NULL)
   {
      return;
   }

   //
   // Free the tile bitmaps.
   //
   if (lpMap->pTileSprite != NULL)
   {
      free(lpMap->pTileSprite);
   }

   //
   // Delete the instance.
   //
   free(lpMap);
}

LPCBITMAPRLE
PAL_MapGetTileBitmap(
   BYTE       x,
   BYTE       y,
   BYTE       h,
   BYTE       ucLayer,
   LPCPALMAP  lpMap
)
/*++
  Purpose:

    Get the tile bitmap on the specified layer at the location (x, y, h).

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

    [IN]  lpMap - Pointer to the loaded map.

  Return value:

    Pointer to the bitmap. NULL if failed.

--*/
{
   DWORD d;

   //
   // Check for invalid parameters.
   //
   if (x >= 64 || y >= 128 || h > 1 || lpMap == NULL)
   {
      return NULL;
   }

   //
   // Get the tile data of the specified location.
   //
   d = lpMap->Tiles[y][x][h];

   if (ucLayer == 0)
   {
      //
      // Bottom layer
      //
      return PAL_SpriteGetFrame(lpMap->pTileSprite, (d & 0xFF) | ((d >> 4) & 0x100));
   }
   else
   {
      //
      // Top layer
      //
      d >>= 16;
      return PAL_SpriteGetFrame(lpMap->pTileSprite, ((d & 0xFF) | ((d >> 4) & 0x100)) - 1);
   }
}

BOOL
PAL_MapTileIsBlocked(
   BYTE       x,
   BYTE       y,
   BYTE       h,
   LPCPALMAP  lpMap
)
/*++
  Purpose:

    Check if the tile at the specified location is blocked.

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  lpMap - Pointer to the loaded map.

  Return value:

    TRUE if the tile is blocked, FALSE if not.

--*/
{
   //
   // Check for invalid parameters.
   //
   if (x >= 64 || y >= 128 || h > 1 || lpMap == NULL)
   {
      return TRUE;
   }

   return (lpMap->Tiles[y][x][h] & 0x2000) >> 13;
}

BYTE
PAL_MapGetTileHeight(
   BYTE       x,
   BYTE       y,
   BYTE       h,
   BYTE       ucLayer,
   LPCPALMAP  lpMap
)
/*++
  Purpose:

    Get the logical height value of the specified tile. This value is used
    to judge whether the tile bitmap should cover the sprites or not.

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

    [IN]  lpMap - Pointer to the loaded map.

  Return value:

    The logical height value of the specified tile.

--*/
{
   DWORD      d;

   //
   // Check for invalid parameters.
   //
   if (y >= 128 || x >= 64 || h > 1 || lpMap == NULL)
   {
      return 0;
   }

   d = lpMap->Tiles[y][x][h];

   if (ucLayer)
   {
      d >>= 16;
   }

   d >>= 8;
   return (BYTE)(d & 0xf);
}

VOID
PAL_MapBlitToSurface(
   LPCPALMAP             lpMap,
   SDL_Surface          *lpSurface,
   const SDL_Rect       *lpSrcRect,
   BYTE                  ucLayer
)
/*++
  Purpose:

    Blit the specified map area to a SDL Surface.

  Parameters:

    [IN]  lpMap - Pointer to the map.

    [OUT] lpSurface - Pointer to the destination surface.

    [IN]  lpSrcRect - Pointer to the source area.

    [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

  Return value:

    None.

--*/
{
   int              sx, sy, dx, dy, x, y, h, xPos, yPos;
   LPCBITMAPRLE     lpBitmap = NULL;

   //
   // Convert the coordinate
   //
   sy = lpSrcRect->y / 16 - 1;
   dy = (lpSrcRect->y + lpSrcRect->h) / 16 + 2;
   sx = lpSrcRect->x / 32 - 1;
   dx = (lpSrcRect->x + lpSrcRect->w) / 32 + 2;

   //
   // Do the drawing.
   //
   yPos = sy * 16 - 8 - lpSrcRect->y;
   for (y = sy; y < dy; y++)
   {
      for (h = 0; h < 2; h++, yPos += 8)
      {
         xPos = sx * 32 + h * 16 - 16 - lpSrcRect->x;
         for (x = sx; x < dx; x++, xPos += 32)
         {
            lpBitmap = PAL_MapGetTileBitmap((BYTE)x, (BYTE)y, (BYTE)h, ucLayer, lpMap);
            if (lpBitmap == NULL)
            {
               if (ucLayer)
               {
                  continue;
               }
               lpBitmap = PAL_MapGetTileBitmap(0, 0, 0, ucLayer, lpMap);
            }
            PAL_RLEBlitToSurface(lpBitmap, lpSurface, PAL_XY(xPos, yPos));
         }
      }
   }
}
