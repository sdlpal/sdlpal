//
// PAL DOS compress format (YJ_1) library
//
// Author: Lou Yihua <louyihua@21cn.com>
//
// Copyright 2006 - 2007 Lou Yihua
//
// This file is part of PAL library.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//

// Ported to C from C++ and modified for compatibility with Big-Endian
// by Wei Mingzhi <whistler@openoffice.org>.

// TODO: fix YJ_2 for big-endian

#include "common.h"

#ifndef PAL_WIN95

typedef struct _TreeNode
{
   unsigned char   value;
   unsigned char   leaf;
   unsigned short  level;
   unsigned int    weight;

   struct _TreeNode *parent;
   struct _TreeNode *left;
   struct _TreeNode *right;
} TreeNode;

typedef struct _TreeNodeList
{
   TreeNode *node;
   struct _TreeNodeList *next;
} TreeNodeList;

typedef struct _YJ_1_FILEHEADER
{
   unsigned int   Signature;          // 'YJ_1'
   unsigned int   UncompressedLength; // size before compression
   unsigned int   CompressedLength;   // size after compression
   unsigned short BlockCount;       // number of blocks
   unsigned char Unknown;
   unsigned char HuffmanTreeLength; // length of huffman tree
} YJ_1_FILEHEADER, *PYJ_1_FILEHEADER;

typedef struct _YJ_1_BLOCKHEADER
{
   unsigned short UncompressedLength; // maximum 0x4000
   unsigned short CompressedLength;   // including the header
   unsigned short LZSSRepeatTable[4];
   unsigned char LZSSOffsetCodeLengthTable[4];
   unsigned char LZSSRepeatCodeLengthTable[3];
   unsigned char CodeCountCodeLengthTable[3];
   unsigned char CodeCountTable[2];
} YJ_1_BLOCKHEADER, *PYJ_1_BLOCKHEADER;

static unsigned int
get_bits(
   const void *src,
   unsigned int *bitptr,
   unsigned int count
)
{
   unsigned char *temp = ((unsigned char *)src) + ((*bitptr >> 4) << 1);
   unsigned int bptr = *bitptr & 0xf;
   unsigned short mask;
   *bitptr += count;
   if (count > 16 - bptr)
   {
      count = count + bptr - 16;
      mask = 0xffff >> bptr;
      return (((temp[0] | (temp[1] << 8)) & mask) << count) | ((temp[2] | (temp[3] << 8)) >> (16 - count));
   }
   else
      return (((unsigned short)((temp[0] | (temp[1] << 8)) << bptr)) >> (16 - count));
}

static unsigned short
get_loop(
   const void *src,
   unsigned int *bitptr,
   PYJ_1_BLOCKHEADER header
)
{
   if (get_bits(src, bitptr, 1))
      return header->CodeCountTable[0];
   else
   {
      unsigned int temp = get_bits(src, bitptr, 2);
      if (temp)
         return get_bits(src, bitptr, header->CodeCountCodeLengthTable[temp - 1]);
      else
         return header->CodeCountTable[1];
   }
}

static unsigned short
get_count(
   const void *src,
   unsigned int *bitptr,
   PYJ_1_BLOCKHEADER header
)
{
   unsigned short temp;
   if ((temp = get_bits(src, bitptr, 2)) != 0)
   {
      if (get_bits(src, bitptr, 1))
         return get_bits(src, bitptr, header->LZSSRepeatCodeLengthTable[temp - 1]);
      else
         return SWAP16(header->LZSSRepeatTable[temp]);
   }
   else
      return SWAP16(header->LZSSRepeatTable[0]);
}

INT
Decompress(
   LPCVOID       Source,
   LPVOID        Destination,
   INT           DestSize
)
{
   PYJ_1_FILEHEADER hdr = (PYJ_1_FILEHEADER)Source;
   unsigned char *src = (unsigned char *)Source;
   unsigned char *dest;
   unsigned int i;
   TreeNode *root, *node;

   if (Source == NULL)
      return -1;
   if (SWAP32(hdr->Signature) != 0x315f4a59)
      return -1;
   if (SWAP32(hdr->UncompressedLength) > (unsigned int)DestSize)
      return -1;

   do
   {
      unsigned short tree_len = ((unsigned short)hdr->HuffmanTreeLength) * 2;
      unsigned int bitptr = 0;
      unsigned char *flag = (unsigned char *)src + 16 + tree_len;

      if ((node = root = (TreeNode *)malloc(sizeof(TreeNode) * (tree_len + 1))) == NULL)
         return -1;
      root[0].leaf = 0;
      root[0].value = 0;
      root[0].left = root + 1;
      root[0].right = root + 2;
      for (i = 1; i <= tree_len; i++)
      {
         root[i].leaf = !get_bits(flag, &bitptr, 1);
         root[i].value = src[15 + i];
         if (root[i].leaf)
            root[i].left = root[i].right = NULL;
         else
         {
            root[i].left =  root + (root[i].value << 1) + 1;
            root[i].right = root[i].left + 1;
         }
      }
      src += 16 + tree_len + (((tree_len & 0xf) ? (tree_len >> 4) + 1 : (tree_len >> 4)) << 1);
   } while (0);

   dest = (unsigned char *)Destination;

   for (i = 0; i < SWAP16(hdr->BlockCount); i++)
   {
      unsigned int bitptr;
      PYJ_1_BLOCKHEADER header;

      header = (PYJ_1_BLOCKHEADER)src;
      src += 4;
      if (!SWAP16(header->CompressedLength))
      {
         unsigned short hul = SWAP16(header->UncompressedLength);
         while (hul--)
         {
            *dest++ = *src++;
         }
         continue;
      }
      src += 20;
      bitptr = 0;
      for (;;)
      {
         unsigned short loop;
         if ((loop = get_loop(src, &bitptr, header)) == 0)
            break;

         while (loop--)
         {
            node = root;
            for(; !node->leaf;)
            {
               if (get_bits(src, &bitptr, 1))
                  node = node->right;
               else
                  node = node->left;
            }
            *dest++ = node->value;
         }

         if ((loop = get_loop(src, &bitptr, header)) == 0)
            break;

         while (loop--)
         {
            unsigned int pos, count;
            count = get_count(src, &bitptr, header);
            pos = get_bits(src, &bitptr, 2);
            pos = get_bits(src, &bitptr, header->LZSSOffsetCodeLengthTable[pos]);
            while (count--)
            {
               *dest = *(dest - pos);
               dest++;
            }
         }
      }
      src = ((unsigned char *)header) + SWAP16(header->CompressedLength);
   }
   free(root);

   return SWAP32(hdr->UncompressedLength);
}

#else

typedef struct _TreeNode
{
   unsigned short      weight;
   unsigned short      value;
   struct _TreeNode*   parent;
   struct _TreeNode*   left;
   struct _TreeNode*   right;
} TreeNode;

typedef struct _Tree
{
   TreeNode*   node;
   TreeNode**   list;
} Tree;

static unsigned char data1[0x100] =
{
0x3f,0x0b,0x17,0x03,0x2f,0x0a,0x16,0x00,0x2e,0x09,0x15,0x02,0x2d,0x01,0x08,0x00,
0x3e,0x07,0x14,0x03,0x2c,0x06,0x13,0x00,0x2b,0x05,0x12,0x02,0x2a,0x01,0x04,0x00,
0x3d,0x0b,0x11,0x03,0x29,0x0a,0x10,0x00,0x28,0x09,0x0f,0x02,0x27,0x01,0x08,0x00,
0x3c,0x07,0x0e,0x03,0x26,0x06,0x0d,0x00,0x25,0x05,0x0c,0x02,0x24,0x01,0x04,0x00,
0x3b,0x0b,0x17,0x03,0x23,0x0a,0x16,0x00,0x22,0x09,0x15,0x02,0x21,0x01,0x08,0x00,
0x3a,0x07,0x14,0x03,0x20,0x06,0x13,0x00,0x1f,0x05,0x12,0x02,0x1e,0x01,0x04,0x00,
0x39,0x0b,0x11,0x03,0x1d,0x0a,0x10,0x00,0x1c,0x09,0x0f,0x02,0x1b,0x01,0x08,0x00,
0x38,0x07,0x0e,0x03,0x1a,0x06,0x0d,0x00,0x19,0x05,0x0c,0x02,0x18,0x01,0x04,0x00,
0x37,0x0b,0x17,0x03,0x2f,0x0a,0x16,0x00,0x2e,0x09,0x15,0x02,0x2d,0x01,0x08,0x00,
0x36,0x07,0x14,0x03,0x2c,0x06,0x13,0x00,0x2b,0x05,0x12,0x02,0x2a,0x01,0x04,0x00,
0x35,0x0b,0x11,0x03,0x29,0x0a,0x10,0x00,0x28,0x09,0x0f,0x02,0x27,0x01,0x08,0x00,
0x34,0x07,0x0e,0x03,0x26,0x06,0x0d,0x00,0x25,0x05,0x0c,0x02,0x24,0x01,0x04,0x00,
0x33,0x0b,0x17,0x03,0x23,0x0a,0x16,0x00,0x22,0x09,0x15,0x02,0x21,0x01,0x08,0x00,
0x32,0x07,0x14,0x03,0x20,0x06,0x13,0x00,0x1f,0x05,0x12,0x02,0x1e,0x01,0x04,0x00,
0x31,0x0b,0x11,0x03,0x1d,0x0a,0x10,0x00,0x1c,0x09,0x0f,0x02,0x1b,0x01,0x08,0x00,
0x30,0x07,0x0e,0x03,0x1a,0x06,0x0d,0x00,0x19,0x05,0x0c,0x02,0x18,0x01,0x04,0x00
};
static unsigned char data2[0x10] =
{
0x08,0x05,0x06,0x04,0x07,0x05,0x06,0x03,0x07,0x05,0x06,0x04,0x07,0x04,0x05,0x03
};

static void adjust_tree(Tree tree, unsigned short value)
{
   TreeNode* node = tree.list[value];
   TreeNode tmp;
   TreeNode* tmp1;
   TreeNode* temp;
   while(node->value != 0x280)
   {
      temp = node + 1;
      while(node->weight == temp->weight)
         temp++;
      temp--;
      if (temp != node)
      {
         tmp1 = node->parent;
         node->parent = temp->parent;
         temp->parent = tmp1;
         if (node->value > 0x140)
         {
            node->left->parent = temp;
            node->right->parent = temp;
         }
         else
            tree.list[node->value] = temp;
         if (temp->value > 0x140)
         {
            temp->left->parent = node;
            temp->right->parent = node;
         }
         else
            tree.list[temp->value] = node;
         tmp = *node; *node = *temp; *temp = tmp;
         node = temp;
      }
      node->weight++;
      node = node->parent;
   }
   node->weight++;
}

static int build_tree(Tree *tree)
{
   int i, ptr;
   TreeNode** list;
   TreeNode* node;
   if ((tree->list = list = (TreeNode **)malloc(sizeof(TreeNode*) * 321)) == NULL)
      return 0;
   if ((tree->node = node = (TreeNode *)malloc(sizeof(TreeNode) * 641)) == NULL)
   {
      free(list);
      return 0;
   }
   memset(list, 0, 321 * sizeof(TreeNode*));
   memset(node, 0, 641 * sizeof(TreeNode));
   for(i = 0; i <= 0x140; i++)
      list[i] = node + i;
   for(i = 0; i <= 0x280; i++)
   {
      node[i].value = i;
      node[i].weight = 1;
   }
   tree->node[0x280].parent = tree->node + 0x280;
   for(i = 0, ptr = 0x141; ptr <= 0x280; i += 2, ptr++)
   {
      node[ptr].left = node + i;
      node[ptr].right = node + i + 1;
      node[i].parent = node[i + 1].parent = node + ptr;
      node[ptr].weight = node[i].weight + node[i + 1].weight;
   }
   return 1;
}

#pragma pack(1)
typedef struct _BitField
{
   unsigned char   b0:   1;
   unsigned char   b1:   1;
   unsigned char   b2:   1;
   unsigned char   b3:   1;
   unsigned char   b4:   1;
   unsigned char   b5:   1;
   unsigned char   b6:   1;
   unsigned char   b7:   1;
} BitField;
#pragma pack()

static int bt(const void* data, unsigned int pos)
{
   BitField* bit = (BitField*)((unsigned char*)data + (pos >> 3));
   switch(pos & 0x7)
   {
   case 0:   return bit->b0;
   case 1:   return bit->b1;
   case 2:   return bit->b2;
   case 3:   return bit->b3;
   case 4:   return bit->b4;
   case 5:   return bit->b5;
   case 6:   return bit->b6;
   case 7:   return bit->b7;
   }
   return 0;
}

static void bit(void* data, unsigned int pos, int set)
{
   BitField* bit = (BitField*)((unsigned char*)data + (pos >> 3));
   switch(pos & 0x7)
   {
   case 0:
      bit->b0 = set;
      break;
   case 1:
      bit->b1 = set;
      break;
   case 2:
      bit->b2 = set;
      break;
   case 3:
      bit->b3 = set;
      break;
   case 4:
      bit->b4 = set;
      break;
   case 5:
      bit->b5 = set;
      break;
   case 6:
      bit->b6 = set;
      break;
   case 7:
      bit->b7 = set;
      break;
   }
}

INT
Decompress(
   LPCVOID       Source,
   LPVOID        Destination,
   INT           DestSize
)
{
   unsigned int len = 0, ptr = 0, Length = 0;
   unsigned char* src = (unsigned char*)Source + 4;
   unsigned char* dest;
   Tree tree;
   TreeNode* node;

   if (Source == NULL)
      return -1;

   if (!build_tree(&tree))
      return -1;

   Length = SWAP32(*((unsigned int*)Source));
   if (Length > DestSize)
      return -1;
   dest = (unsigned char*)Destination;

   while (1)
   {
      unsigned short val;
      node = tree.node + 0x280;
      while(node->value > 0x140)
      {
         if (bt(src, ptr))
            node = node->right;
         else
            node = node->left;
         ptr++;
      }
      val = node->value;
      if (tree.node[0x280].weight == 0x8000)
      {
         int i;
         for(i = 0; i < 0x141; i++)
            if (tree.list[i]->weight & 0x1)
               adjust_tree(tree, i);
         for(i = 0; i <= 0x280; i++)
            tree.node[i].weight >>= 1;
      }
      adjust_tree(tree, val);
      if (val > 0xff)
      {
         int i;
         unsigned int temp, tmp, pos;
         unsigned char* pre;
         for(i = 0, temp = 0; i < 8; i++, ptr++)
            temp |= (unsigned int)bt(src, ptr) << i;
         tmp = temp & 0xff;
         for(; i < data2[tmp & 0xf] + 6; i++, ptr++)
            temp |= (unsigned int)bt(src, ptr) << i;
         temp >>= data2[tmp & 0xf];
         pos = (temp & 0x3f) | ((unsigned int)data1[tmp] << 6);
         if (pos == 0xfff)
            break;
         pre = dest - pos - 1;
         for(i = 0; i < val - 0xfd; i++)
            *dest++ = *pre++;
         len += val - 0xfd;
      }
      else
      {
         *dest++ = (unsigned char)val;
         len++;
      }
   }

   free(tree.list);
   free(tree.node);
   return Length;
}

#endif
