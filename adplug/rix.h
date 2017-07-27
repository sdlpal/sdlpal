/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * rix.h - Softstar RIX OPL Format Player by palxex <palxex.ys168.com>
 *                                           BSPAL <BSPAL.ys168.com>
 */

#include "player.h"

#if defined(_MSC_VER) && _MSC_VER <= 1600
#include <windows.h>
#define uint8_t BYTE
#define uint16_t WORD
#define uint32_t DWORD
#else
#include <stdint.h>
#endif

class CrixPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CrixPlayer(Copl *newopl);
  ~CrixPlayer();

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  void rewindReInit(int subsong, bool reinit); /* For seamless continous */
  float getrefresh();
  unsigned int getsubsongs();

  std::string gettype()
    { return std::string("Softstar RIX OPL Music Format"); };

#if USE_RIX_EXTRA_INIT
  void set_extra_init(uint32_t* regs, uint8_t* datas, int n);
#endif

 protected:	
  typedef struct {
    uint8_t v[14];
  } ADDT;

  int flag_mkf;
#if USE_RIX_EXTRA_INIT
  uint32_t *extra_regs;
  uint8_t *extra_vals;
#endif
  FILE *fp;
  int subsongs;
  uint8_t rix_buf[16384];  /* rix files' f_buffer */
  uint16_t f_buffer[300];//9C0h-C18h
  uint16_t a0b0_data2[11];
  uint8_t a0b0_data3[18];
  uint8_t a0b0_data4[18];
  uint8_t a0b0_data5[96];
  uint8_t addrs_head[96];
  uint16_t insbuf[28];
  uint16_t displace[11];
  ADDT reg_bufs[18];
  uint32_t pos, length;
#if USE_RIX_EXTRA_INIT
  uint32_t extra_length;
#endif

  static const uint8_t adflag[18];
  static const uint8_t reg_data[18];
  static const uint8_t ad_C0_offs[18];
  static const uint8_t modify[28];
  static const uint8_t bd_reg_data[124];
  static uint8_t for40reg[18];
  static const uint16_t mus_time;
  uint32_t I,T;
  uint16_t mus_block;
  uint16_t ins_block;
  uint8_t rhythm;
  uint8_t music_on;
  uint8_t pause_flag;
  uint16_t band;
  uint8_t band_low;
  uint16_t e0_reg_flag;
  uint8_t bd_modify;
  int sustain;
  int play_end;

#define ad_08_reg() ad_bop(8,0)    /**/
  void ad_20_reg(uint16_t);              /**/
  void ad_40_reg(uint16_t);              /**/
  void ad_60_reg(uint16_t);              /**/
  void ad_80_reg(uint16_t);              /**/
  void ad_a0b0_reg(uint16_t);            /**/
  void ad_a0b0l_reg(uint16_t,uint16_t,uint16_t); /**/
  void ad_a0b0l_reg_(uint16_t,uint16_t,uint16_t); /**/
  void ad_bd_reg();                  /**/
  void ad_bop(uint16_t,uint16_t);                     /**/
  void ad_C0_reg(uint16_t);              /**/
  void ad_E0_reg(uint16_t);              /**/
  uint16_t ad_initial();                 /**/
  uint16_t ad_test();                    /**/
  void crc_trans(uint16_t,uint16_t);         /**/
  void data_initial();               /* done */
  void init();                       /**/
  void ins_to_reg(uint16_t,uint16_t*,uint16_t);  /**/
  void int_08h_entry();    /**/
  void music_ctrl();                 /**/
  void Pause();                      /**/
  void prepare_a0b0(uint16_t,uint16_t);      /**/
  void rix_90_pro(uint16_t);             /**/
  void rix_A0_pro(uint16_t,uint16_t);        /**/
  void rix_B0_pro(uint16_t,uint16_t);        /**/
  void rix_C0_pro(uint16_t,uint16_t);        /**/
  void rix_get_ins();                /**/
  uint16_t rix_proc();                   /**/
  void set_new_int();
  void switch_ad_bd(uint16_t);           /**/
};
