/*
 *      The OPL-3 mode is switched on by writing 0x01, to the offset 5
 *      of the right side.
 *
 *      Another special register at the right side is at offset 4. It contains
 *      a bit mask defining which voices are used as 4 OP voices.
 *
 *      The percussive mode is implemented in the left side only.
 *
 *      With the above exceptions the both sides can be operated independently.
 * 
 *      A 4 OP voice can be created by setting the corresponding
 *      bit at offset 4 of the right side.
 *
 *      For example setting the rightmost bit (0x01) changes the
 *      first voice on the right side to the 4 OP mode. The fourth
 *      voice is made inaccessible.
 *
 *      If a voice is set to the 2 OP mode, it works like 2 OP modes
 *      of the original YM3812 (AdLib). In addition the voice can 
 *      be connected the left, right or both stereo channels. It can
 *      even be left unconnected. This works with 4 OP voices also.
 *
 *      The stereo connection bits are located in the FEEDBACK_CONNECTION
 *      register of the voice (0xC0-0xC8). In 4 OP voices these bits are
 *      in the second half of the voice.
 */

/*
 *      Register numbers for the global registers
 */

#define TEST_REGISTER                           0x01
#define   ENABLE_WAVE_SELECT            0x20

#define TIMER1_REGISTER                         0x02
#define TIMER2_REGISTER                         0x03
#define TIMER_CONTROL_REGISTER                  0x04    /* Left side */
#define   IRQ_RESET                     0x80
#define   TIMER1_MASK                   0x40
#define   TIMER2_MASK                   0x20
#define   TIMER1_START                  0x01
#define   TIMER2_START                  0x02

#define CONNECTION_SELECT_REGISTER              0x04    /* Right side */
#define   RIGHT_4OP_0                   0x01
#define   RIGHT_4OP_1                   0x02
#define   RIGHT_4OP_2                   0x04
#define   LEFT_4OP_0                    0x08
#define   LEFT_4OP_1                    0x10
#define   LEFT_4OP_2                    0x20

#define OPL3_MODE_REGISTER                      0x05    /* Right side */
#define   OPL3_ENABLE                   0x01
#define   OPL4_ENABLE                   0x02

#define KBD_SPLIT_REGISTER                      0x08    /* Left side */
#define   COMPOSITE_SINE_WAVE_MODE      0x80            /* Don't use with OPL-3? */
#define   KEYBOARD_SPLIT                0x40

#define PERCOSSION_REGISTER                     0xbd    /* Left side only */
#define   TREMOLO_DEPTH                 0x80
#define   VIBRATO_DEPTH                 0x40
#define   PERCOSSION_ENABLE             0x20
#define   BASSDRUM_ON                   0x10
#define   SNAREDRUM_ON                  0x08
#define   TOMTOM_ON                     0x04
#define   CYMBAL_ON                     0x02
#define   HIHAT_ON                      0x01

/*
 *      Offsets to the register banks for operators. To get the
 *      register number just add the operator offset to the bank offset
 *
 *      AM/VIB/EG/KSR/Multiple (0x20 to 0x35)
 */
enum{
	AM_VIB = 0x20,
	TREMOLO_ON = 0x80,
	VIBRATO_ON = 0x40,
	SUSTAIN_ON = 0x20,
	KSR = 0x10,	/* Key scaling rate */
	MULTIPLE_MASK = 0x0f	/* Frequency multiplier */
};

 /*
  *     KSL/Total level (0x40 to 0x55)
  */
#define KSL_LEVEL                               0x40
#define   KSL_MASK                      0xc0    /* Envelope scaling bits */
#define   TOTAL_LEVEL_MASK              0x3f    /* Strength (volume) of OP */

/*
 *      Attack / Decay rate (0x60 to 0x75)
 */
#define ATTACK_DECAY                            0x60
#define   ATTACK_MASK                   0xf0
#define   DECAY_MASK                    0x0f

/*
 * Sustain level / Release rate (0x80 to 0x95)
 */
#define SUSTAIN_RELEASE                         0x80
#define   SUSTAIN_MASK                  0xf0
#define   RELEASE_MASK                  0x0f

/*
 * Wave select (0xE0 to 0xF5)
 */
#define WAVE_SELECT                     0xe0

/*
 *      Offsets to the register banks for voices. Just add to the
 *      voice number to get the register number.
 *
 *      F-Number low bits (0xA0 to 0xA8).
 */
#define FNUM_LOW                                0xa0

/*
 *      F-number high bits / Key on / Block (octave) (0xB0 to 0xB8)
 */
#define KEYON_BLOCK                                     0xb0
#define   KEYON_BIT                             0x20
#define   BLOCKNUM_MASK                         0x1c
#define   FNUM_HIGH_MASK                        0x03

/*
 *      Feedback / Connection (0xc0 to 0xc8)
 *
 *      These registers have two new bits when the OPL-3 mode
 *      is selected. These bits controls connecting the voice
 *      to the stereo channels. For 4 OP voices this bit is
 *      defined in the second half of the voice (add 3 to the
 *      register offset).
 *
 *      For 4 OP voices the connection bit is used in the
 *      both halves (gives 4 ways to connect the operators).
 */
#define FEEDBACK_CONNECTION                             0xc0
#define   FEEDBACK_MASK                         0x0e    /* Valid just for 1st OP of a voice */
#define   CONNECTION_BIT                        0x01
/*
 *      In the 4 OP mode there is four possible configurations how the
 *      operators can be connected together (in 2 OP modes there is just
 *      AM or FM). The 4 OP connection mode is defined by the rightmost
 *      bit of the FEEDBACK_CONNECTION (0xC0-0xC8) on the both halves.
 *
 *      First half      Second half     Mode
 *
 *                                       +---+
 *                                       v   |
 *      0               0               >+-1-+--2--3--4-->
 *
 *
 *                               
 *                                       +---+
 *                                       |   |
 *      0               1               >+-1-+--2-+
 *                                                |->
 *                                      >--3----4-+
 *                               
 *                                       +---+
 *                                       |   |
 *      1               0               >+-1-+-----+
 *                                                 |->
 *                                      >--2--3--4-+
 *
 *                                       +---+
 *                                       |   |
 *      1               1               >+-1-+--+
 *                                              |
 *                                      >--2--3-+->
 *                                              |
 *                                      >--4----+
 */
#define   STEREO_BITS                           0x30    /* OPL-3 only */
#define     VOICE_TO_LEFT               0x10
#define     VOICE_TO_RIGHT              0x20

enum{
	MAXINST = 100,	/* FIXME: what the fuck is this? */
	MAXPATN = 100,
	MAXCHN = 15  /* Remember that the percussion channel is reserved! */
};

typedef struct MusPlayerPos MusPlayerPos;
typedef struct Pattern Pattern;
typedef struct InternalHdr InternalHdr;
typedef struct InternalSample InternalSample;
typedef struct MusData MusData;

struct MusPlayerPos{
	int Ord;
	int Pat;
	int Row;
	char *Pos;
	int Wait;
};

struct Pattern{
	ushort Len;
	char *Ptr;
};

struct InternalHdr{
    ushort OrdNum;      /* Number of orders, is even                */
                      /* If bit 8 (&256) is set, there exist no   */
                      /* notes between 126..253 and the 7th bit   */
                      /* of note value can be used to tell if     */
                      /* there is instrument number.              */
    ushort InsNum;      /* Number of instruments                    */
    ushort PatNum;      /* Number of patterns                       */
                      /* If bit 9 set, all instruments have       */
                      /* the insname[] filled with AsciiZ name.   */
    ushort Ins2Num;     /* Number of instruments - for checking!    */
    uchar InSpeed;     /* Initial speed in 7 lowermost bits        */
                      /* If highest bit (&128) set, no logaritmic */
                      /* adlib volume conversion needs to be done */
                      /* when playing with midi device.           */
    uchar InTempo;     /* Initial tempo                            */
    uchar ChanCount[];	/* ChanCount[0] present if PatNum bit 9 set */
}; /* Size: 2+2+2+2+1+1 = 8+2 = 10 */

struct InternalSample{
	/* unused */
	char FT;     /*finetune value         1         */
						/*  signed seminotes.              */
	uchar D[11];         /*Identificators         11        */
						/* - 12nd byte not needed.         */
						/* six high bits of D[8]           */
						/* define the midi volume          */
						/* scaling level                   */
						/* - default is 63.                */
						/* D[9] bits 5..2 have             */
						/* the automatical SDx             */
						/* adjust value.                   */
						/* - default is 0.                 */
                        /* D[9] bits 6-7 are free.         */
	char Volume;        /*0..63                  1         */
						/* If bit 6 set,                   */
						/*  the instrument will be         */
						/*  played simultaneously          */
						/*  on two channels when           */
						/*  playing with a midi device.    */
						/*  To gain more volume.           */
						/* If bit 7 set,                   */
						/*  the finetune value             */
						/*  affects also FM.               */
						/*  (SMP->AME conversion)          */
	ushort C2Spd;         /*8363 = normal..?       2         */
	uchar GM;            /*GM program number      1         */
    uchar Bank;          /*Bank number, 0=normal. 1         */
                        /*Highest bit (&128) is free.      */
    char insname[];
	                    /*Only if PatNum&512 set           */
};        /*             total:17 = 0x11     */

struct MusData{
    int PlayNext;
        /* Set to 1 every time next line is played.
           Use it to syncronize screen with music. */

    long RowTimer;
        /* You can delay the music with this.
           Or fasten. Units: Rate */

    char *Playing; /* Currently playing music from this buffer. */
    int Paused;    /* If paused, interrupts normally but does not play. *
                    * Read only. There's PauseS3M() for this.           */
    InternalHdr *Hdr;                
	/* FIXME: >127 for percussion; was set to MAXINST before, causing bad reads; is
	   this used correctly now? why is it 100? what the fuck is it for??? */
    InternalSample *Instr[2*MAXINST+1];

    int LinearMidiVol;
    int GlobalVolume;

    int Speed;
    int Tempo;

    /* Data from song */
    char *Orders;
    Pattern Patn[MAXPATN];

    /* Player: For SBx */
    MusPlayerPos posi;
	MusPlayerPos saved;
    int LoopCount;
    int PendingSB0;
    
    long CurVol[MAXCHN];
    uchar NNum[MAXCHN];
    uchar CurPuh[MAXCHN];
    int NoteCut[MAXCHN];
    int NoteDelay[MAXCHN];
    uchar CurInst[MAXCHN];
    uchar InstNum[MAXCHN]; /* For the instrument number optimizer */
    uchar PatternDelay;

    char FineVolSlide[MAXCHN];
    char FineFrqSlide[MAXCHN];
    char  VolSlide[MAXCHN];
    char FreqSlide[MAXCHN];
     uchar VibDepth[MAXCHN];
     uchar VibSpd  [MAXCHN];
     uchar VibPos  [MAXCHN];
    uchar ArpegSpd;
    uchar Arpeggio[MAXCHN];
    uchar ArpegCnt[MAXCHN];
     uchar Retrig[MAXCHN];

    char Active[MAXCHN];
    char Doubles[MAXCHN];

    uchar DxxDefault[MAXCHN];
    uchar ExxDefault[MAXCHN];
    uchar HxxDefault[MAXCHN];
	uchar MxxDefault[MAXCHN];
	uchar SxxDefault[MAXCHN];

    ulong Herz[MAXCHN];  /* K�ytt�� m_opl */
};
#define FxxDefault ExxDefault /* They really are combined in ST3 */

/* fmopl */
enum{
	OPL_SAMPLE_BITS = 16
};

typedef s16int OPLSAMPLE;	//INT32
typedef void	(*OPL_TIMERHANDLER)(void *param, int timer, double interval_sec);
typedef void	(*OPL_IRQHANDLER)(void *param, int irq);
typedef void	(*OPL_UPDATEHANDLER)(void *param, int min_interval_us);
typedef void	(*OPL_PORTHANDLER_W)(void *param, uchar data);
typedef uchar	(*OPL_PORTHANDLER_R)(void *param);

typedef struct OPL_SLOT OPL_SLOT;
typedef struct OPL_CH OPL_CH;
typedef struct FM_OPL FM_OPL;

struct OPL_SLOT{
	u32int  ar;         /* attack rate: AR<<2           */
	u32int  dr;         /* decay rate:  DR<<2           */
	u32int  rr;         /* release rate:RR<<2           */
	u8int   KSR;        /* key scale rate               */
	u8int   ksl;        /* keyscale level               */
	u8int   ksr;        /* key scale rate: kcode>>KSR   */
	u8int   mul;        /* multiple: mul_tab[ML]        */

	/* Phase Generator */
	u32int  Cnt;        /* frequency counter            */
	u32int  Incr;       /* frequency counter step       */
	u8int   FB;         /* feedback shift value         */
	s32int   *connect1;  /* slot1 output pointer         */
	s32int   op1_out[2]; /* slot1 output for feedback    */
	u8int   CON;        /* connection (algorithm) type  */

	/* Envelope Generator */
	u8int   eg_type;    /* percussive/non-percussive mode */
	u8int   state;      /* phase type                   */
	u32int  TL;         /* total level: TL << 2         */
	s32int   TLL;        /* adjusted now TL              */
	s32int   volume;     /* envelope counter             */
	u32int  sl;         /* sustain level: sl_tab[SL]    */
	u8int   eg_sh_ar;   /* (attack state)               */
	u8int   eg_sel_ar;  /* (attack state)               */
	u8int   eg_sh_dr;   /* (decay state)                */
	u8int   eg_sel_dr;  /* (decay state)                */
	u8int   eg_sh_rr;   /* (release state)              */
	u8int   eg_sel_rr;  /* (release state)              */
	u32int  key;        /* 0 = KEY OFF, >0 = KEY ON     */

	/* LFO */
	u32int  AMmask;     /* LFO Amplitude Modulation enable mask */
	u8int   vib;        /* LFO Phase Modulation enable flag (active high)*/

	/* waveform select */
	u16int  wavetable;
};
struct OPL_CH{
	OPL_SLOT SLOT[2];
	/* phase generator state */
	u32int  block_fnum; /* block+fnum                   */
	u32int  fc;         /* Freq. Increment base         */
	u32int  ksl_base;   /* KeyScaleLevel Base step      */
	u8int   kcode;      /* key code (for key scaling)   */
};
struct FM_OPL{
	/* FM channel slots */
	OPL_CH  P_CH[9];                /* OPL/OPL2 chips have 9 channels*/

	u32int  eg_cnt;                 /* global envelope generator counter    */
	u32int  eg_timer;               /* global envelope generator counter works at frequency = chipclock/72 */
	u32int  eg_timer_add;           /* step of eg_timer                     */
	u32int  eg_timer_overflow;      /* envelope generator timer overlfows every 1 sample (on real chip) */

	u8int   rhythm;                 /* Rhythm mode                  */

	u32int  fn_tab[1024];           /* fnumber->increment counter   */

	/* LFO */
	u32int  LFO_AM;
	s32int   LFO_PM;

	u8int   lfo_am_depth;
	u8int   lfo_pm_depth_range;
	u32int  lfo_am_cnt;
	u32int  lfo_am_inc;
	u32int  lfo_pm_cnt;
	u32int  lfo_pm_inc;

	u32int  noise_rng;              /* 23 bit noise shift register  */
	u32int  noise_p;                /* current noise 'phase'        */
	u32int  noise_f;                /* current noise period         */

	u8int   wavesel;                /* waveform select enable flag  */

	u32int  T[2];                   /* timer counters               */
	u8int   st[2];                  /* timer enable                 */

	/* external event callback handlers */
	OPL_TIMERHANDLER  timer_handler;    /* TIMER handler                */
	void *TimerParam;                   /* TIMER parameter              */
	OPL_IRQHANDLER    IRQHandler;   /* IRQ handler                  */
	void *IRQParam;                 /* IRQ parameter                */
	OPL_UPDATEHANDLER UpdateHandler;/* stream update handler        */
	void *UpdateParam;              /* stream update parameter      */

	u8int type;                     /* chip type                    */
	u8int address;                  /* address register             */
	u8int status;                   /* status flag                  */
	u8int statusmask;               /* status mask                  */
	u8int mode;                     /* Reg.08 : CSM,notesel,etc.    */

	u32int clock;                   /* master clock  (Hz)           */
	u32int rate;                    /* sampling rate (Hz)           */
	double freqbase;                /* frequency base               */
	double TimerBase;         /* Timer base time (==sampling time)*/

	s32int phase_modulation;    /* phase modulation input (SLOT 2) */
	s32int output[1];
};
