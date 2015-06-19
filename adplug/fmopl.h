#ifndef __FMOPL_H_
#define __FMOPL_H_

#define HAS_YM3812	1

/* --- select emulation chips --- */
#define BUILD_YM3812 (HAS_YM3812)
#define BUILD_YM3526 (HAS_YM3526)
#define BUILD_Y8950  (HAS_Y8950)

/* select output bits size of output : 8 or 16 */
#define OPL_SAMPLE_BITS 16

/* compiler dependence */
#ifndef OSD_CPU_H
#define OSD_CPU_H
typedef unsigned char	UINT8;   /* unsigned  8bit */
typedef unsigned short	UINT16;  /* unsigned 16bit */
typedef unsigned int	UINT32;  /* unsigned 32bit */
typedef signed char		INT8;    /* signed  8bit   */
typedef signed short	INT16;   /* signed 16bit   */
typedef signed int		INT32;   /* signed 32bit   */
#endif

#if (OPL_SAMPLE_BITS==16)
typedef INT16 OPLSAMPLE;
#endif
#if (OPL_SAMPLE_BITS==8)
typedef INT8 OPLSAMPLE;
#endif


typedef void (*OPL_TIMERHANDLER)(int channel,double interval_Sec);
typedef void (*OPL_IRQHANDLER)(int param,int irq);
typedef void (*OPL_UPDATEHANDLER)(int param,int min_interval_us);
typedef void (*OPL_PORTHANDLER_W)(int param,unsigned char data);
typedef unsigned char (*OPL_PORTHANDLER_R)(int param);



#define OPL_TYPE_WAVESEL   0x01  /* waveform select		*/
#define OPL_TYPE_ADPCM     0x02  /* DELTA-T ADPCM unit	*/
#define OPL_TYPE_KEYBOARD  0x04  /* keyboard interface	*/
#define OPL_TYPE_IO        0x08  /* I/O port			*/

/* ---------- Generic interface section ---------- */
#define OPL_TYPE_YM3526 (0)
#define OPL_TYPE_YM3812 (OPL_TYPE_WAVESEL)
#define OPL_TYPE_Y8950  (OPL_TYPE_ADPCM|OPL_TYPE_KEYBOARD|OPL_TYPE_IO)


typedef struct{
	UINT32	ar;			/* attack rate: AR<<2			*/
	UINT32	dr;			/* decay rate:  DR<<2			*/
	UINT32	rr;			/* release rate:RR<<2			*/
	UINT8	KSR;		/* key scale rate				*/
	UINT8	ksl;		/* keyscale level				*/
	UINT8	ksr;		/* key scale rate: kcode>>KSR	*/
	UINT8	mul;		/* multiple: mul_tab[ML]		*/

	/* Phase Generator */
	UINT32	Cnt;		/* frequency counter			*/
	UINT32	Incr;		/* frequency counter step		*/
	UINT8   FB;			/* feedback shift value			*/
	INT32   *connect1;	/* slot1 output pointer			*/
	INT32   op1_out[2];	/* slot1 output for feedback	*/
	UINT8   CON;		/* connection (algorithm) type	*/

	/* Envelope Generator */
	UINT8	eg_type;	/* percussive/non-percussive mode */
	UINT8	state;		/* phase type					*/
	UINT32	TL;			/* total level: TL << 2			*/
	INT32	TLL;		/* adjusted now TL				*/
	INT32	volume;		/* envelope counter				*/
	UINT32	sl;			/* sustain level: sl_tab[SL]	*/
	UINT8	eg_sh_ar;	/* (attack state)				*/
	UINT8	eg_sel_ar;	/* (attack state)				*/
	UINT8	eg_sh_dr;	/* (decay state)				*/
	UINT8	eg_sel_dr;	/* (decay state)				*/
	UINT8	eg_sh_rr;	/* (release state)				*/
	UINT8	eg_sel_rr;	/* (release state)				*/
	UINT32	key;		/* 0 = KEY OFF, >0 = KEY ON		*/

	/* LFO */
	UINT32	AMmask;		/* LFO Amplitude Modulation enable mask */
	UINT8	vib;		/* LFO Phase Modulation enable flag (active high)*/

	/* waveform select */
	unsigned int wavetable;
} OPL_SLOT;

typedef struct{
	OPL_SLOT SLOT[2];
	/* phase generator state */
	UINT32  block_fnum;	/* block+fnum					*/
	UINT32  fc;			/* Freq. Increment base			*/
	UINT32  ksl_base;	/* KeyScaleLevel Base step		*/
	UINT8   kcode;		/* key code (for key scaling)	*/
} OPL_CH;

/* OPL state */
typedef struct fm_opl_f {
	/* FM channel slots */
	OPL_CH	P_CH[9];				/* OPL/OPL2 chips have 9 channels*/

	UINT32	eg_cnt;					/* global envelope generator counter	*/
	UINT32	eg_timer;				/* global envelope generator counter works at frequency = chipclock/72 */
	UINT32	eg_timer_add;			/* step of eg_timer						*/
	UINT32	eg_timer_overflow;		/* envelope generator timer overlfows every 1 sample (on real chip) */

	UINT8	rhythm;					/* Rhythm mode					*/

	UINT32	fn_tab[1024];			/* fnumber->increment counter	*/

	/* LFO */
	UINT8	lfo_am_depth;
	UINT8	lfo_pm_depth_range;
	UINT32	lfo_am_cnt;
	UINT32	lfo_am_inc;
	UINT32	lfo_pm_cnt;
	UINT32	lfo_pm_inc;

	UINT32	noise_rng;				/* 23 bit noise shift register	*/
	UINT32	noise_p;				/* current noise 'phase'		*/
	UINT32	noise_f;				/* current noise period			*/

	UINT8	wavesel;				/* waveform select enable flag	*/

	int		T[2];					/* timer counters				*/
	UINT8	st[2];					/* timer enable					*/

#if BUILD_Y8950
	/* Delta-T ADPCM unit (Y8950) */

	YM_DELTAT *deltat;

	/* Keyboard and I/O ports interface */
	UINT8	portDirection;
	UINT8	portLatch;
	OPL_PORTHANDLER_R porthandler_r;
	OPL_PORTHANDLER_W porthandler_w;
	int		port_param;
	OPL_PORTHANDLER_R keyboardhandler_r;
	OPL_PORTHANDLER_W keyboardhandler_w;
	int		keyboard_param;
#endif

	/* external event callback handlers */
	OPL_TIMERHANDLER  TimerHandler;	/* TIMER handler				*/
	int TimerParam;					/* TIMER parameter				*/
	OPL_IRQHANDLER    IRQHandler;	/* IRQ handler					*/
	int IRQParam;					/* IRQ parameter				*/
	OPL_UPDATEHANDLER UpdateHandler;/* stream update handler		*/
	int UpdateParam;				/* stream update parameter		*/

	UINT8 type;						/* chip type					*/
	UINT8 address;					/* address register				*/
	UINT8 status;					/* status flag					*/
	UINT8 statusmask;				/* status mask					*/
	UINT8 mode;						/* Reg.08 : CSM,notesel,etc.	*/

	int clock;						/* master clock  (Hz)			*/
	int rate;						/* sampling rate (Hz)			*/
	double freqbase;				/* frequency base				*/
	double TimerBase;				/* Timer base time (==sampling time)*/
} FM_OPL;
extern FM_OPL *OPLCreate(int type, int clock, int rate);
extern void OPLDestroy(FM_OPL *OPL);
extern int OPLWrite(FM_OPL *OPL,int a,int v);
extern void OPLResetChip(FM_OPL *OPL);

#if BUILD_YM3812

int  YM3812Init(int num, int clock, int rate);
void YM3812Shutdown(void);
void YM3812ResetChip(int which);
int  YM3812Write(int which, int a, int v);
unsigned char YM3812Read(int which, int a);
int  YM3812TimerOver(int which, int c);
void YM3812UpdateOne(FM_OPL		*OPL, INT16 *buffer, int length);

void YM3812SetTimerHandler(int which, OPL_TIMERHANDLER TimerHandler, int channelOffset);
void YM3812SetIRQHandler(int which, OPL_IRQHANDLER IRQHandler, int param);
void YM3812SetUpdateHandler(int which, OPL_UPDATEHANDLER UpdateHandler, int param);

#endif


#if BUILD_YM3526

/*
** Initialize YM3526 emulator(s).
**
** 'num' is the number of virtual YM3526's to allocate
** 'clock' is the chip clock in Hz
** 'rate' is sampling rate
*/
int  YM3526Init(int num, int clock, int rate);
/* shutdown the YM3526 emulators*/
void YM3526Shutdown(void);
void YM3526ResetChip(int which);
int  YM3526Write(int which, int a, int v);
unsigned char YM3526Read(int which, int a);
int  YM3526TimerOver(int which, int c);
/*
** Generate samples for one of the YM3526's
**
** 'which' is the virtual YM3526 number
** '*buffer' is the output buffer pointer
** 'length' is the number of samples that should be generated
*/
void YM3526UpdateOne(int which, INT16 *buffer, int length);

void YM3526SetTimerHandler(int which, OPL_TIMERHANDLER TimerHandler, int channelOffset);
void YM3526SetIRQHandler(int which, OPL_IRQHANDLER IRQHandler, int param);
void YM3526SetUpdateHandler(int which, OPL_UPDATEHANDLER UpdateHandler, int param);

#endif


#if BUILD_Y8950

/* Y8950 port handlers */
void Y8950SetPortHandler(int which, OPL_PORTHANDLER_W PortHandler_w, OPL_PORTHANDLER_R PortHandler_r, int param);
void Y8950SetKeyboardHandler(int which, OPL_PORTHANDLER_W KeyboardHandler_w, OPL_PORTHANDLER_R KeyboardHandler_r, int param);
void Y8950SetDeltaTMemory(int which, void * deltat_mem_ptr, int deltat_mem_size );

int  Y8950Init (int num, int clock, int rate);
void Y8950Shutdown (void);
void Y8950ResetChip (int which);
int  Y8950Write (int which, int a, int v);
unsigned char Y8950Read (int which, int a);
int  Y8950TimerOver (int which, int c);
void Y8950UpdateOne (int which, INT16 *buffer, int length);

void Y8950SetTimerHandler (int which, OPL_TIMERHANDLER TimerHandler, int channelOffset);
void Y8950SetIRQHandler (int which, OPL_IRQHANDLER IRQHandler, int param);
void Y8950SetUpdateHandler (int which, OPL_UPDATEHANDLER UpdateHandler, int param);

#endif


#endif /* __FMOPL_H_ */
