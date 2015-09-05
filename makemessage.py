#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import with_statement

import argparse
import os
import traceback
import struct

def main():

    parser = argparse.ArgumentParser(description = 'Generate a translatable language file that can be used by SDLPAL.')
    parser.add_argument('gamepath', help = 'Game path where SSS.MKF & M.MSG & WORD.DAT are located.')
    parser.add_argument('outputfile', help = 'Path of the output message file.')
    parser.add_argument('encoding', choices = ['gbk', 'big5'], help = 'Text encoding name, should be either gbk or big5.')
    parser.add_argument('-w', '--width', dest = 'wordwidth', default = 10, type = int, help = 'Word width in bytes, default is 10')
    parser.add_argument("-c", "--comment", action = 'store_true', help = 'Automatically generate comments')
    options = parser.parse_args()

    if options.gamepath[-1] != '/' and options.gamepath[-1] != '\\':
        options.gamepath += '/'

    script_bytes = []
    index_bytes = []
    msg_bytes = []
    word_bytes = []

    is_msg_group = 0    #是否正在处理文字组的标示。
    msg_count = 0
    last_index = -1
    temp = ""
    comment = ""
    message = ""

    for file_ in os.listdir(options.gamepath):
        if file_.lower() == 'sss.mkf':
            try:
                with open(options.gamepath + file_, 'rb') as f:
                    f.seek(12, os.SEEK_SET)
                    offset_begin, script_begin, file_end = struct.unpack('<III', f.read(12))
                    f.seek(offset_begin, os.SEEK_SET)
                    index_bytes = f.read(script_begin - offset_begin)
                    script_bytes = f.read(file_end - script_begin)
            except:
                traceback.print_exc()
                return
        elif file_.lower() == 'm.msg':
            try:
                with open(options.gamepath + file_, 'rb') as f:
                    msg_bytes = f.read()
            except:
                traceback.print_exc()
                return
        elif file_.lower() == 'word.dat':
            try:
                with open(options.gamepath + file_, 'rb') as f:
                    data_bytes=f.read()
            except:
                traceback.print_exc()
                return

    if len(data_bytes) % options.wordwidth != 0:
        data_bytes += [0x20 for i in range(0, options.wordwidth - len(data_bytes) % options.wordwidth)]

    output = "# All lines, except those inside [BEIGN MESSAGE] and [END MESSAGE], can be commented by adding the sharp '#' mark at the first of the line.\n\n"

    output += "# This section contains the information that will be displayed when a user finishes the game.\n"
    output += "# Only the keys listed here are valid. Other keys will be ignored.\n"
    output += "[BEGIN CREDITS]\n"
    output += "# Place the translated text of 'Classical special build' here in no more than 24 half-wide characters.\n"
    output += "1= Classical special build\n"
    output += "# Place the translated porting information template at the following two lines. Be aware that each replaced line will be truncated into at most 40 half-wide characters.\n"
    output += "6= ${platform} port by ${author}, ${year}.\n"
    output += "7=\n"
    output += "# Place the translated GNU licensing information at the following three lines. Be aware that each line will be truncated into at most 40 half-wide characters.\n"
    output += "8=   This is a free software and it is\n"
    output += "9=   published under GNU General Public\n"
    output += "10=    License v3.\n"
    output += "# Place the translated text at the following line. Be aware that each line will be truncated into at most 40 half-wide characters.\n"
    output += "11=    ...Press Enter to continue\n"
    output += "[END CREDITS]\n\n"

    output += "# Each line controls one position value, lines from 1 to 26 is for the equipment menu.\n"
    output += "# For each line, the format is 'idx=x,y,flag', while the flag is optional\n"
    output += "# If bit 0 of flag is 1, then use 8x8 font; and while bit 1 of flag is 1, then disable shadow\n"
    output += "[BEGIN LAYOUT]\n"
    output += "# 1 is the position of image box in equip menu\n"
    output += "1=8,8\n"
    output += "# 2 is the position of role list box in equip menu\n"
    output += "2=2,95\n"
    output += "# 3 is the position of current equipment's name in equip menu\n"
    output += "3=5,70\n"
    output += "# 4 is the position of current equipment's amount in equip menu\n"
    output += "4=51,57\n"
    output += "# 5 .. 10 are the positions of words 566 ... 571 in equip menu\n"
    output += "5=92,11\n"
    output += "6=92,33\n"
    output += "7=92,55\n"
    output += "8=92,77\n"
    output += "9=92,99\n"
    output += "10=92,121\n"
    output += "# 11 .. 16 are the positions of equipped equipments in equip menu\n"
    output += "11=130,11\n"
    output += "12=130,33\n"
    output += "13=130,55\n"
    output += "14=130,77\n"
    output += "15=130,99\n"
    output += "16=130,121\n"
    output += "# 17 .. 21 are the positions of words 51 ... 55 in equip menu\n"
    output += "17=226,10\n"
    output += "18=226,32\n"
    output += "19=226,54\n"
    output += "20=226,76\n"
    output += "21=226,98\n"
    output += "# 22 .. 26 are the positions of status values in equip menu\n"
    output += "22=260,14\n"
    output += "23=260,36\n"
    output += "24=260,58\n"
    output += "25=260,80\n"
    output += "26=260,102\n"
    output += "[END LAYOUT]\n\n"

    output += "# This section contains the words used by the game.\n"
    output += "[BEGIN WORDS]\n"
    output += "# Each line is a pattern of 'key=value', where key is an integer and value is a string.\n"
    for i in range(0, len(data_bytes) / options.wordwidth):
        temp = data_bytes[i * options.wordwidth: (i + 1) * options.wordwidth].rstrip('\x20\x00').decode(options.encoding).encode('utf-8')
        if options.comment: output += "# Original word: %d=%s\n" % (i, temp)
        output += "%d=%s\n" % (i, temp)
    output += "566=Headgear\n"
    output += "567=Body Gear\n"
    output += "568=Clothing\n"
    output += "569=Weapon\n"
    output += "570=Footwear\n"
    output += "571=Accessory\n"
    output += "# This is the only addtional word for ATB in SDLPAL. It is not used in classical mode.\n"
    output += "65530=Battle Speed\n"
    output += "[END WORDS]\n\n"

    output += "# The following sections contain dialog/description texts used by the game.\n\n"

    print "Now Processing. Please wait..."
    
    for i in range(0, len(script_bytes) / 8):
        op, w1, w2, w3 = struct.unpack('<HHHH', script_bytes[i * 8 : (i + 1) * 8])
        if op == 0xFFFF:

            if is_msg_group == 0:
                is_msg_group = 1
                message = "%s %d\n" % ('[BEGIN MESSAGE]', w1)
                if options.comment: comment = "# Original message: %d\n" % w1

            last_index = w1
            msg_count += 1
            msg_begin, msg_end = struct.unpack("<II",index_bytes[w1 * 4 : (w1 + 2) * 4])

            try:
                temp = "%s\n" % (msg_bytes[msg_begin : msg_end].decode(options.encoding, 'replace').encode('utf-8'))
                message += temp
                if options.comment: comment += "# " + temp
            except:
                traceback.print_exc()

        elif op == 0x008E:

            if is_msg_group == 1:
                temp = "%s\n" % ('[CLEAR MESSAGE]')
                message += temp
                if options.comment: comment += "# " + temp

        else:
            if is_msg_group == 1:
                is_msg_group = 0
                temp = "%s %d\n\n" % ('[END MESSAGE]', last_index)
                message += temp
                output += comment + message

    try:
        with open(options.outputfile, "wt") as f:
            f.write(output)
    except:
        traceback.print_exc()

    print "OK! Extraction finished!"
    print "Original Dialog script count: " + str(msg_count)

if __name__ == '__main__':
    main()
