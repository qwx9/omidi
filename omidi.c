#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

enum{
	NCHIPS = 1,
	RATE = 44100
};
/* FIXME: 8192 bytes or shorts? */
static short abuf[8192];
static short *ap;
static int afd = -1;

/* FIXME: using multiple chips */
/* FIXME: the only reason this exists and FM_OPL is defined in dat.h instead of being
 * static in fmopl.c is to have multiple cards... and we don't support that; either
 * implement it and it's cool and it should be that way, or nuke this */
static FM_OPL *chip[NCHIPS];
static int oplretval;
static int oplregno;
static double otm;

enum{
	MINDELAY = 1,
	OPLBASE = 0x388
};

static int verbose;

static struct MusData mdat;

int ColorNums = -1;

const long Period[12] =
{
    907,960,1016,1076,
    1140,1208,1280,1356,
    1440,1524,1616,1712
};

static const char Portit[9] = {0,1,2, 8,9,10, 16,17,18};

static signed char Pans[18] = {0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0};

signed char Trig[18] = {0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0};

//#define SetBase(a) ((a)==2 ? (a) : 9)
#define SetBase(a) (a)

static uchar FMVol[65] = {
    /* Generated similarly as LogTable */
    0,32,48,58,64,70,74,77,80,83,86,
    88,90,92,93,95,96,98,99,100,102,
    103,104,105,106,107,108,108,109,
    110,111,112,112,113,114,114,115,
    116,116,117,118,118,119,119,120,
    120,121,121,122,122,123,123,124,
    124,124,125,125,126,126,126,127,
    127,127,128,128
};

static uchar adl[] = {
    14,0,0,1,143,242,244,0,8,1,6,242,247,0,14,0,0,1,75,242,244,0,8,1,0,242,247,0,14,0,0,1,73,242,244,0,8,1,0,242,246,0,14,0,0,129,18,242,247,0,6,65,0,242,247,0,14,0,0,1,87,241,247,0,0,1,0,242,247,0,14,0,0,1,147,241,247,0,0,1,0,242,247,0,14,0,0,1,128,161,
    242,0,8,22,14,242,245,0,14,0,0,1,146,194,248,0,10,1,0,194,248,0,14,0,0,12,92,246,244,0,0,129,0,243,245,0,14,0,0,7,151,243,242,0,2,17,128,242,241,0,14,0,0,23,33,84,244,0,2,1,0,244,244,0,14,0,0,152,98,243,246,0,0,129,0,242,246,0,14,0,0,24,35,246,246,
    0,0,1,0,231,247,0,14,0,0,21,145,246,246,0,4,1,0,246,246,0,14,0,0,69,89,211,243,0,12,129,128,163,243,0,14,0,0,3,73,117,245,1,4,129,128,181,245,0,14,0,0,113,146,246,20,0,2,49,0,241,7,0,14,0,0,114,20,199,88,0,2,48,0,199,8,0,14,0,0,112,68,170,24,0,4,177,
    0,138,8,0,14,0,0,35,147,151,35,1,4,177,0,85,20,0,14,0,0,97,19,151,4,1,0,177,128,85,4,0,14,0,0,36,72,152,42,1,12,177,0,70,26,0,14,0,0,97,19,145,6,1,10,33,0,97,7,0,14,0,0,33,19,113,6,0,6,161,137,97,7,0,14,0,0,2,156,243,148,1,12,65,128,243,200,0,14,0,
    0,3,84,243,154,1,12,17,0,241,231,0,14,0,0,35,95,241,58,0,0,33,0,242,248,0,14,0,0,3,135,246,34,1,6,33,128,243,248,0,14,0,0,3,71,249,84,0,0,33,0,246,58,0,14,0,0,35,74,145,65,1,8,33,5,132,25,0,14,0,0,35,74,149,25,1,8,33,0,148,25,0,14,0,0,9,161,32,79,
    0,8,132,128,209,248,0,14,0,0,33,30,148,6,0,2,162,0,195,166,0,14,0,0,49,18,241,40,0,10,49,0,241,24,0,14,0,0,49,141,241,232,0,10,49,0,241,120,0,14,0,0,49,91,81,40,0,12,50,0,113,72,0,14,0,0,1,139,161,154,0,8,33,64,242,223,0,14,0,0,33,139,162,22,0,8,33,
    8,161,223,0,14,0,0,49,139,244,232,0,10,49,0,241,120,0,14,0,0,49,18,241,40,0,10,49,0,241,24,0,14,0,0,49,21,221,19,1,8,33,0,86,38,0,14,0,0,49,22,221,19,1,8,33,0,102,6,0,14,0,0,113,73,209,28,1,8,49,0,97,12,0,14,0,0,33,77,113,18,1,2,35,128,114,6,0,14,
    0,0,241,64,241,33,1,2,225,0,111,22,0,14,0,0,2,26,245,117,1,0,1,128,133,53,0,14,0,0,2,29,245,117,1,0,1,128,243,244,0,14,0,0,16,65,245,5,1,2,17,0,242,195,0,14,0,0,33,155,177,37,1,14,162,1,114,8,0,14,0,0,161,152,127,3,1,0,33,0,63,7,1,14,0,0,161,147,193,
    18,0,10,97,0,79,5,0,14,0,0,33,24,193,34,0,12,97,0,79,5,0,14,0,0,49,91,244,21,0,0,114,131,138,5,0,14,0,0,161,144,116,57,0,0,97,0,113,103,0,14,0,0,113,87,84,5,0,12,114,0,122,5,0,14,0,0,144,0,84,99,0,8,65,0,165,69,0,14,0,0,33,146,133,23,0,12,33,1,143,
    9,0,14,0,0,33,148,117,23,0,12,33,5,143,9,0,14,0,0,33,148,118,21,0,12,97,0,130,55,0,14,0,0,49,67,158,23,1,2,33,0,98,44,1,14,0,0,33,155,97,106,0,2,33,0,127,10,0,14,0,0,97,138,117,31,0,8,34,6,116,15,0,14,0,0,161,134,114,85,1,0,33,131,113,24,0,14,0,0,
    33,77,84,60,0,8,33,0,166,28,0,14,0,0,49,143,147,2,1,8,97,0,114,11,0,14,0,0,49,142,147,3,1,8,97,0,114,9,0,14,0,0,49,145,147,3,1,10,97,0,130,9,0,14,0,0,49,142,147,15,1,10,97,0,114,15,0,14,0,0,33,75,170,22,1,8,33,0,143,10,0,14,0,0,49,144,126,23,1,6,33,
    0,139,12,1,14,0,0,49,129,117,25,1,0,50,0,97,25,0,14,0,0,50,144,155,33,0,4,33,0,114,23,0,14,0,0,225,31,133,95,0,0,225,0,101,26,0,14,0,0,225,70,136,95,0,0,225,0,101,26,0,14,0,0,161,156,117,31,0,2,33,0,117,10,0,14,0,0,49,139,132,88,0,0,33,0,101,26,0,
    14,0,0,225,76,102,86,0,0,161,0,101,38,0,14,0,0,98,203,118,70,0,0,161,0,85,54,0,14,0,0,98,153,87,7,0,11,161,0,86,7,0,14,0,0,98,147,119,7,0,11,161,0,118,7,0,14,0,0,34,89,255,3,2,0,33,0,255,15,0,14,0,0,33,14,255,15,1,0,33,0,255,15,1,14,0,0,34,70,134,
    85,0,0,33,128,100,24,0,14,0,0,33,69,102,18,0,0,161,0,150,10,0,14,0,0,33,139,146,42,1,0,34,0,145,42,0,14,0,0,162,158,223,5,0,2,97,64,111,7,0,14,0,0,32,26,239,1,0,0,96,0,143,6,2,14,0,0,33,143,241,41,0,10,33,128,244,9,0,14,0,0,119,165,83,148,0,2,161,
    0,160,5,0,14,0,0,97,31,168,17,0,10,177,128,37,3,0,14,0,0,97,23,145,52,0,12,97,0,85,22,0,14,0,0,113,93,84,1,0,0,114,0,106,3,0,14,0,0,33,151,33,67,0,8,162,0,66,53,0,14,0,0,161,28,161,119,1,0,33,0,49,71,1,14,0,0,33,137,17,51,0,10,97,3,66,37,0,14,0,0,
    161,21,17,71,1,0,33,0,207,7,0,14,0,0,58,206,248,246,0,2,81,0,134,2,0,14,0,0,33,21,33,35,1,0,33,0,65,19,0,14,0,0,6,91,116,149,0,0,1,0,165,114,0,14,0,0,34,146,177,129,0,12,97,131,242,38,0,14,0,0,65,77,241,81,1,0,66,0,242,245,0,14,0,0,97,148,17,81,1,
    6,163,128,17,19,0,14,0,0,97,140,17,49,0,6,161,128,29,3,0,14,0,0,164,76,243,115,1,4,97,0,129,35,0,14,0,0,2,133,210,83,0,0,7,3,242,246,1,14,0,0,17,12,163,17,1,0,19,128,162,229,0,14,0,0,17,6,246,65,1,4,17,0,242,230,2,14,0,0,147,145,212,50,0,8,145,0,235,
    17,1,14,0,0,4,79,250,86,0,12,1,0,194,5,0,14,0,0,33,73,124,32,0,6,34,0,111,12,1,14,0,0,49,133,221,51,1,10,33,0,86,22,0,14,0,0,32,4,218,5,2,6,33,129,143,11,0,14,0,0,5,106,241,229,0,6,3,128,195,229,0,14,0,0,7,21,236,38,0,10,2,0,248,22,0,14,0,0,5,157,
    103,53,0,8,1,0,223,5,0,14,0,0,24,150,250,40,0,10,18,0,248,229,0,14,0,0,16,134,168,7,0,6,0,3,250,3,0,14,0,0,17,65,248,71,2,4,16,3,243,3,0,14,0,0,1,142,241,6,2,14,16,0,243,2,0,14,0,0,14,0,31,0,0,14,192,0,31,255,3,14,0,0,6,128,248,36,0,14,3,136,86,132,
    2,14,0,0,14,0,248,0,0,14,208,5,52,4,3,14,0,0,14,0,246,0,0,14,192,0,31,2,3,14,0,0,213,149,55,163,0,0,218,64,86,55,0,14,0,0,53,92,178,97,2,10,20,8,244,21,0,14,0,0,14,0,246,0,0,14,208,0,79,245,3,14,0,0,38,0,255,1,0,14,228,0,18,22,1,14,0,0,0,0,243,240,
    0,14,0,0,246,201,2,14,0,35,16,68,248,119,2,8,17,0,243,6,0,14,0,35,16,68,248,119,2,8,17,0,243,6,0,14,0,52,2,7,249,255,0,8,17,0,248,255,0,14,0,48,0,0,252,5,2,14,0,0,250,23,0,14,0,58,0,2,255,7,0,0,1,0,255,8,0,14,0,60,0,0,252,5,2,14,0,0,250,23,0,14,0,
    47,0,0,246,12,0,4,0,0,246,6,0,14,0,43,12,0,246,8,0,10,18,0,251,71,2,14,0,49,0,0,246,12,0,4,0,0,246,6,0,14,0,43,12,0,246,8,0,10,18,5,123,71,2,14,0,51,0,0,246,12,0,4,0,0,246,6,0,14,0,43,12,0,246,2,0,10,18,0,203,67,2,14,0,54,0,0,246,12,0,4,0,0,246,6,
    0,14,0,57,0,0,246,12,0,4,0,0,246,6,0,14,0,72,14,0,246,0,0,14,208,0,159,2,3,14,0,60,0,0,246,12,0,4,0,0,246,6,0,14,0,76,14,8,248,66,0,14,7,74,244,228,3,14,0,84,14,0,245,48,0,14,208,10,159,2,0,14,0,36,14,10,228,228,3,6,7,93,245,229,1,14,0,65,2,3,180,
    4,0,14,5,10,151,247,0,14,0,84,78,0,246,0,0,14,158,0,159,2,3,14,0,83,17,69,248,55,2,8,16,8,243,5,0,14,0,84,14,0,246,0,0,14,208,0,159,2,3,14,0,24,128,0,255,3,3,12,16,13,255,20,0,14,0,77,14,8,248,66,0,14,7,74,244,228,3,14,0,60,6,11,245,12,0,6,2,0,245,
    8,0,14,0,65,1,0,250,191,0,7,2,0,200,151,0,14,0,59,1,81,250,135,0,6,1,0,250,183,0,14,0,51,1,84,250,141,0,6,2,0,248,184,0,14,0,45,1,89,250,136,0,6,2,0,248,182,0,14,0,71,1,0,249,10,3,14,0,0,250,6,0,14,0,60,0,128,249,137,3,14,0,0,246,108,0,14,0,58,3,128,
    248,136,3,15,12,8,246,182,0,14,0,53,3,133,248,136,3,15,12,0,246,182,0,14,0,64,14,64,118,79,0,14,0,8,119,24,2,14,0,71,14,64,200,73,0,14,3,0,155,105,2,14,0,61,215,220,173,5,3,14,199,0,141,5,0,14,0,61,215,220,168,4,3,14,199,0,136,4,0,14,0,44,128,0,246,
    6,3,14,17,0,103,23,3,14,0,40,128,0,245,5,2,14,17,9,70,22,3,14,0,69,6,63,0,244,0,1,21,0,247,245,0,14,0,68,6,63,0,244,3,0,18,0,247,245,0,14,0,63,6,63,0,244,0,1,18,0,247,245,0,14,0,74,1,88,103,231,0,0,2,0,117,7,0,14,0,60,65,69,248,72,0,0,66,8,117,5,0,
    14,0,80,10,64,224,240,3,8,30,78,255,5,0,14,0,64,10,124,224,240,3,8,30,82,255,2,0,14,0,72,14,64,122,74,0,14,0,8,123,27,2,14,0,73,14,10,228,228,3,6,7,64,85,57,1,14,0,70,5,5,249,50,3,14,4,64,214,165,0,14,0,68,2,63,0,243,3,8,21,0,247,245,0,14,0,48,1,79,
    250,141,0,7,2,0,248,181,0,14,0,53,0,0,246,12,0,4,0,0,246,6,0
};


static void
pcmout(int v)
{
	if(v < -32768)
		v = -32768;
	if(v > 32767)
		v = 32767;
	/* FIXME: portability (endianness) */
	*ap++ = v;
	/* FIXME: stereo shit */
	*ap++ = v;

	/* FIXME: write last samples before end */
	if(ap >= abuf+nelem(abuf)){
		write(afd, abuf, sizeof abuf);
		ap = abuf;
	}
}

static uint
tadv(double length, double add)
{
	add += otm;
	otm = fmod(add, length);

	return add / length;
}

static void
mix(uint usecs)
{
	int i;
	uint n;
	s16int *buf;

	n = tadv((double)1.0 / RATE, (double)usecs / 1E6);
	if((buf = mallocz(n * sizeof *buf, 1)) == nil)
		sysfatal("mallocz: %r");

	ym3812_update_one(chip[0], buf, n);
	for(i = 0; i < n; i++)
		//pcmout((double)buf[i] / 32768.0 * 10000.0);	/* FIXME: why? */
		pcmout(buf[i]);

	free(buf);
}

static uchar
inb(uint port)
{
	if(port >= 0x388 && port <= 0x38b)
		return oplretval;
	return 0;
}

static void
outb(uint port, uchar v)
{
	uint ind;

	if(port >= 0x388 && port <= 0x38b){
		ind = port - 0x388;
		ym3812_write(chip[0], ind, v);

		if(ind & 1){
			if(oplregno == 4){
				if(v == 0x80)
					oplretval = 0x02;
				else if(v == 0x21)
					oplretval = 0xc0;
			}
		}else
			oplregno = v;
	}
}

static void
OPL_Byte(uchar Index, uchar Data)
{
	int a;

	outb(OPLBASE, Index);
	for(a=0; a<6; a++)
		inb(OPLBASE);

	outb(OPLBASE+1, Data);
	for(a=0; a<35; a++)
		inb(OPLBASE);
}

static void
OPL_NoteOff(int c)
{
    Trig[c] = 0;

    c = SetBase(c);
    if(c<9)
    {
        int Ope = Portit[c];
        /* KEYON_BLOCK+c seems to not work alone?? */
        OPL_Byte(KEYON_BLOCK+c, 0);
        OPL_Byte(KSL_LEVEL+  Ope, 0xFF);
        OPL_Byte(KSL_LEVEL+3+Ope, 0xFF);
    }
}

/* OPL_NoteOn changes the frequency on specified
   channel and guarantees the key is on. (Doesn't
   retrig, just turns the note on and sets freq.) */
/* Could be used for pitch bending also. */
static void OPL_NoteOn(int c, unsigned long Herz)
{
    int Oct;

    Trig[c] = 127;

    c = SetBase(c);
    if(c >= 9)return;

    for(Oct=0; Herz>0x1FF; Oct++)Herz >>= 1;

/*
    Bytes A0-B8 - Octave / F-Number / Key-On

        7     6     5     4     3     2     1     0
     +-----+-----+-----+-----+-----+-----+-----+-----+
     |        F-Number (least significant byte)      |  (A0-A8)
     +-----+-----+-----+-----+-----+-----+-----+-----+
     |  Unused   | Key |    Octave       | F-Number  |  (B0-B8)
     |           | On  |                 | most sig. |
     +-----+-----+-----+-----+-----+-----+-----+-----+
*/

    OPL_Byte(0xA0+c, Herz&255);  //F-Number low 8 bits
    OPL_Byte(0xB0+c, 0x20        //Key on
                      | ((Herz>>8)&3) //F-number high 2 bits
                      | ((Oct&7)<<2)
          );
}

static void
OPL_Touch(int c, int Instru, ushort Vol)
{
    int Ope;
    //int level;

    c = SetBase(c);
    if(c >= 9)return;

    Ope = Portit[c];

/*
    Bytes 40-55 - Level Key Scaling / Total Level

        7     6     5     4     3     2     1     0
     +-----+-----+-----+-----+-----+-----+-----+-----+
     |  Scaling  |             Total Level           |
     |   Level   | 24    12     6     3    1.5   .75 | <-- dB
     +-----+-----+-----+-----+-----+-----+-----+-----+
          bits 7-6 - causes output levels to decrease as the frequency
                     rises:
                          00   -  no change
                          10   -  1.5 dB/8ve
                          01   -  3 dB/8ve
                          11   -  6 dB/8ve
          bits 5-0 - controls the total output level of the operator.
                     all bits CLEAR is loudest; all bits SET is the
                     softest.  Don't ask me why.
*/
/* if 1 */
	OPL_Byte(KSL_LEVEL+  Ope, (mdat.Instr[Instru]->D[2]&KSL_MASK)
	| (63 + (mdat.Instr[Instru]->D[2]&63) * Vol / 63 - Vol));
	OPL_Byte(KSL_LEVEL+3+Ope, (mdat.Instr[Instru]->D[3]&KSL_MASK)
	| (63 + (mdat.Instr[Instru]->D[3]&63) * Vol / 63 - Vol));
/* else
    level = (mdat.Instr[Instru]->D[2]&63) - (Vol*72-8);
    if(level<0)level=0;
    if(level>63)level=63;

    OPL_Byte(KSL_LEVEL+  Ope, (mdat.Instr[Instru]->D[2]&KSL_MASK) | level);

    level = (mdat.Instr[Instru]->D[3]&63) - (Vol*72-8);
    if(level<0)level=0;
    if(level>63)level=63;

    OPL_Byte(KSL_LEVEL+3+Ope, (mdat.Instr[Instru]->D[3]&KSL_MASK) | level);
   endif */
}

static void OPL_Pan(int c, uchar val)
{
    Pans[c] = val - 128;
}

static void OPL_Patch(int c, int Instru)
{
    int Ope;

    c = SetBase(c);
    if(c >= 9)return;

    Ope = Portit[c];

	if(Instru > sizeof(mdat.Instr)-1)
		sysfatal("invalid instrument patch %ud", Instru);	/* FIXME: see dat.h comments */

    OPL_Byte(AM_VIB+           Ope, mdat.Instr[Instru]->D[0]);
    OPL_Byte(ATTACK_DECAY+     Ope, mdat.Instr[Instru]->D[4]);
    OPL_Byte(SUSTAIN_RELEASE+  Ope, mdat.Instr[Instru]->D[6]);
    OPL_Byte(WAVE_SELECT+      Ope, mdat.Instr[Instru]->D[8]&3);// 6 high bits used elsewhere

    OPL_Byte(AM_VIB+         3+Ope, mdat.Instr[Instru]->D[1]);
    OPL_Byte(ATTACK_DECAY+   3+Ope, mdat.Instr[Instru]->D[5]);
    OPL_Byte(SUSTAIN_RELEASE+3+Ope, mdat.Instr[Instru]->D[7]);
    OPL_Byte(WAVE_SELECT+    3+Ope, mdat.Instr[Instru]->D[9]&3);// 6 high bits used elsewhere

    /* Panning... */
    OPL_Byte(FEEDBACK_CONNECTION+c, 
        (mdat.Instr[Instru]->D[10] & ~STEREO_BITS)
            | (Pans[c]<-32 ? VOICE_TO_LEFT
                : Pans[c]>32 ? VOICE_TO_RIGHT
                : (VOICE_TO_LEFT | VOICE_TO_RIGHT)
            ) );
}

/* u32int, word and byte have been defined in adlib.h */

static u32int ConvL(u8int *s)
{
    return (((u32int)s[0] << 24) | ((u32int)s[1] << 16) | ((u16int)s[2] << 8) | s[3]);
}

static u16int ConvI(u8int *s)
{
    return (s[0] << 8) | s[1];
}

static struct Midi
{
/* Fixed by ReadMIDI() */
    int Fmt;
    int TrackCount;
    int DeltaTicks;
    ulong *TrackLen;
    u8int **Tracks;
/* Used by play() */
    u32int  *Waiting, *sWaiting, *SWaiting;
    u8int   *Running, *sRunning, *SRunning;
    ulong *Posi, *sPosi, *SPosi;
    u32int  Tempo;
    u32int  oldtempo;
    u32int  initempo;
/* Per channel */
    u8int   Pan[16];
    u8int   Patch[16];
    u8int   MainVol[16];
    u8int   PitchSense[16];
    int    Bend[16];
    int    OldBend[16];
    int    Used[16][127]; /* contains references to adlib channels per note */
} MIDI;

#define snNoteOff    0x7fb1
#define snNoteOn     0x7fb2
#define snNoteModify 0x7fb3
#define snNoteUpdate 0x7fb4

enum{
	MAXS3MCHAN = 9
};

typedef struct
{
/* Important - at offset 0 to be saved to file */
    long Age;
    u8int Note;
    u8int Instru;
    u8int Volume;
    u8int cmd;
    u8int info;
    u8int KeyFlag;    /* Required to diff aftertouch and noteon */
                     /* Byte to save space */
/* Real volume(0..127) and main volume(0..127) */
    int RealVol;
    int MainVol;
    int exp;	/* vol = MainVol * RealVol/127 * exp/127 */
    /* RealVol must be first non-saved */
/* Less important */
    int LastInstru;  /* Needed by SuggestNewChan() */
    int BendFlag;
    u8int LastPan;
/* To fasten forcement */
    int uChn, uNote;
} S3MChan;

static S3MChan Chan[MAXCHN];
static S3MChan PrevChan[MAXCHN];

#define chansavesize ((int)((long)&Chan[0].RealVol - (long)&Chan[0].Age))

static int InstruUsed[256];
static int Forced=0;
static const int tempochanged = -5;


static void AnalyzeRow(void)
{
    int a;
    
    for(a=0; a<MAXS3MCHAN; a++)
        if(!Chan[a].Age)break;

    if(a==MAXS3MCHAN)
	return;

    memcpy(PrevChan, Chan, sizeof PrevChan);

    Forced=0;

    for(a=0; a<MAXS3MCHAN; a++)
        Chan[a].KeyFlag=0;
}

static void FixSpeed(float *Speed, int *Tempo)
{
    int a;
    float tmp = 1.0;
    *Speed = MIDI.Tempo * 4E-7 / MIDI.DeltaTicks;
    *Tempo = 125;

    for(a=0x40; a<=0xFF; a++)
    {
        double Tmp;
        float n = a * *Speed;
        n = modf(1.0/n, &Tmp);
        if(n < tmp)
        {
            *Tempo = a;
            tmp = n;
        }
    }
    *Speed *= *Tempo;
}

static int MakeVolume(int vol)
{
    return FMVol[vol*64/127]*63/128;
}

static void Bendi(int chn, int a)
{
	int bc, nt;
	long HZ1, HZ2, Herz;

	bc = MIDI.Bend[chn];
	nt = Chan[a].Note;

	if(bc > MIDI.OldBend[chn])
		Chan[a].BendFlag |= 1;
	else if(bc < MIDI.OldBend[chn])
		Chan[a].BendFlag |= 2;
	else
		Chan[a].BendFlag = 0;

	MIDI.OldBend[chn] = bc;

	for(; bc < 0; bc += 0x1000)
		nt--;
	for(; bc >= 0x1000; bc -= 0x1000)
		nt++;

	HZ1 = Period[(nt+1)%12] * (8363L << (nt+1)/12) / 44100U;
	HZ2 = Period[(nt  )%12] * (8363L << (nt  )/12) / 44100U;

	Herz = HZ2 + bc * (HZ1 - HZ2) / 0x1000;

	OPL_NoteOn(a, Herz);
}

/* vole = RealVol*MainVol/127 */
static int SuggestNewChan(int instru, int /*vole*/)
{
    int a, c=MAXS3MCHAN, f;
    long b;

    for(a=f=0; a<MAXS3MCHAN; a++)
        if(Chan[a].LastInstru==instru)
            f=1;

    /* Arvostellaan channels */
    for(b=a=0; a<MAXS3MCHAN; a++)
    {
        /* empty if channel is silent */
        if(!Chan[a].Volume)
        {
            long d;
            /* Pohjapisteet...
             * Jos instru oli uusi, mieluiten sijoitetaan
             * sellaiselle kanavalle, joka on pitk��n ollut hiljaa.
             * Muuten sille, mill� se juuri �skenkin soi.
             */
            d = f?1:Chan[a].Age;

            /* Jos kanavan edellinen instru oli joku
             * soinniltaan hyvin lyhyt, pisteit� annetaan lis�� */
            if(strchr("\x81\x82\x83\x84\x85\x86\x87\x88\x89"
                      "\x8B\x8D\x8E\x90\x94\x96\x9A\x9B\x9C"
                      "\x9D\xA4\xAA\xAB",
                Chan[a].LastInstru))d += 2;
            else
            {
                /* Jos oli pitk�sointinen percussion, *
                 * annetaan pisteit� i�n mukaan.      */
                if(Chan[a].LastInstru > 0x80)
                    d += Chan[a].Age*2;
            }

            /* Jos oli samaa instrua, pisteit� tulee paljon lis�� */
            if(Chan[a].LastInstru == instru)d += 3;

            //d = (d-1)*Chan[a].Age+1;
            if(d > b)
            {
                b = d;
                c = a;
            }
        }
    }
    return c;
}

/* vole = RealVol*MainVol/127 */
static int ForceNewChan(int instru, int vole)
{
    int a, c;
    long b=0;

    vole *= 127;

    Forced=1;
    for(a=c=0; c<MAXS3MCHAN; c++)
        if(Chan[c].Age
          > b
          + (((instru<128 && Chan[c].Instru>128)
             || (vole > Chan[c].RealVol*Chan[c].MainVol)
             ) ? 1:0
          ) )
        {
            a=c;
            b=Chan[c].Age;
        }
    return a;
}

/* Used twice by SetNote. This should be considered as a macro. */
static void SubNoteOff(int a, int chn, int note)
{
    Chan[a].RealVol = 0;
    Chan[a].MainVol = 0;
    Chan[a].exp = 0x7f;

    Chan[a].Age     = 0;
    Chan[a].Volume  = 0;
    Chan[a].BendFlag= 0;

    MIDI.Used[chn][note] = 0;

    if(Chan[a].Instru < 0x80)OPL_NoteOff(a);
}

static void SetNote(int chn, int note, int RealVol,int MainVol, int bend, int e)
{
	int a, vole;

	//vole = RealVol*(MainVol*e)/127;
	vole = RealVol*(MainVol)/127;

    if(!vole && (bend==snNoteOn || bend==snNoteModify))bend=snNoteOff;
    if(bend==snNoteOn && MIDI.Used[chn][note])bend=snNoteModify;

    switch(bend)
    {
        /* snNoteOn:ssa note ei koskaan ole -1 */
        case snNoteOn:
        {
            int p;

            /* FIXME */
            p = chn==9 ? 128+note-35 : MIDI.Patch[chn];

            a = SuggestNewChan(p, vole);

            if(a==MAXS3MCHAN)
            {
                a = ForceNewChan(p, vole);
                MIDI.Used[Chan[a].uChn][Chan[a].uNote] = 0;
            }

            if(a < MAXS3MCHAN)
            {
		Chan[a].exp = e;
                Chan[a].RealVol= RealVol;
                Chan[a].MainVol= MainVol;
                Chan[a].Note   = chn==9 ? 60 : note;
                Chan[a].Volume = MakeVolume(vole);
                Chan[a].Age    = 0;
                Chan[a].Instru = p;

                Chan[a].LastInstru = p;
                Chan[a].KeyFlag= 1;
                Chan[a].uChn  = chn;
                Chan[a].uNote = note;

/*
                if(MIDI.Bend[chn] && Chan[a].Volume)
                {
                    int adlbend = MIDI.Bend[chn] * 127L / 8000;

                    //TODO: Fix this. It doesn't work at all.

                    Chan[a].cmd = 'N'-64;
                    Chan[a].info= adlbend+0x80; // To make it unsigned
                }
                else
*/
                if(MIDI.Pan[chn] != Chan[a].LastPan && Chan[a].Volume)
                {
                    Chan[a].cmd = 'X'-64;
                    Chan[a].info= MIDI.Pan[chn]*2;
                    Chan[a].LastPan = MIDI.Pan[chn];
                }
                else
                {
                    Chan[a].cmd = Chan[a].info = 0;
                }

                OPL_NoteOff(a);

                OPL_Patch(a, Chan[a].Instru);
                if(MIDI.Pan[chn] != 64)OPL_Pan(a, Chan[a].info);
                OPL_Touch(a, Chan[a].Instru, Chan[a].Volume);

                Bendi(chn, a);

                MIDI.Used[chn][note] = a+1;
            }
            break;
        }
        /* snNoteOff:ssa note voi olla -1 */
        case snNoteOff:
        {
            int b=note, c=note;
            if(note < 0)b=0, c=127;

            MIDI.Bend[chn] = 0; /* N�in vaikuttaisi olevan hyv� */

            for(note=b; note<=c; note++)
            {
                a = MIDI.Used[chn][note];
                if(a > 0)
                    SubNoteOff(a-1, chn, note);
            }
            break;
        }
        /* snNoteModify:ssa note ei koskaan ole -1 */
        case snNoteModify:
            a = MIDI.Used[chn][note]-1;
            if(a != -1)
            {
		Chan[a].exp = e;
                Chan[a].RealVol= RealVol;
                Chan[a].MainVol= MainVol;

                Chan[a].Volume = MakeVolume(vole);
                Chan[a].Age    = 0;

                OPL_Touch(a, Chan[a].Instru, Chan[a].Volume);
            }
            break;
        /* snNoteUpdate:ssa note on aina -1 */
        case snNoteUpdate:
            /* snNoteUpdatessa RealVol ei muutu, vain MainVol */
            for(note=0; note<=127; note++)
            {
                a = MIDI.Used[chn][note]-1;

                //vole = MainVol*(Chan[a].RealVol*Chan[a].exp)/127;
                vole = MainVol*(Chan[a].RealVol)/127;

                if(a >= 0 && Chan[a].Volume != MakeVolume(vole))
                {
                    Chan[a].MainVol= MainVol;

                    if(!vole)
                        SubNoteOff(a, chn, note);
                    else
                    {
                        Chan[a].Volume = MakeVolume(vole);
                        Chan[a].Age    = 0;
                        OPL_Touch(a, Chan[a].Instru, Chan[a].Volume);
                    }
                }
            }
            break;
        /* Bendiss� note on aina -1 */
        default:
            MIDI.Bend[chn] = bend;

            for(note=0; note<=127; note++)
            {
                a = MIDI.Used[chn][note]-1;
                if(a >= 0)
                    Bendi(chn, a);
            }
    }
}

static u32int GetVarLen(int Track, ulong *Posi)
{
    u32int d = 0;
    for(;;)
    {
        u8int b = MIDI.Tracks[Track][(*Posi)++];
        d = (d<<7) + (b&127);
        if(b < 128)break;
    }
    return d;
}

/* NOTICE: This copies len-2 bytes */
static char *Sana(int len, const unsigned char *buf)
{
    static char Buf[128];
    char *s = Buf;
    len -= 2;
#define add(c) if(s<&Buf[sizeof(Buf)-1])*s++=(c)
    while(len>0)
    {
        if(*buf<32||(*buf>=127&&*buf<160)){add('^');add(*buf + 64);}
        else add(*buf);
        buf++;
        len--;
    }
#undef add
    *s=0;
    return Buf;
}

static void
MPU_Byte(uchar)
{

}

static void
readmeta(int t, u8int *p, ulong n)
{
	u8int b;

	b = *p;
	p += 2;
	//fprint(2, "meta %ux\n", b);
	switch(b){
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		if(verbose)
			fprint(2, "%s\n", Sana(n, p));
		break;
	case 0x2f:
		MIDI.Posi[t] = MIDI.TrackLen[t];
		if(verbose)
			fprint(2, "end\n");
		break;
	case 0x51:
		MIDI.Tempo = p[0]<<16 | p[1]<<8 | p[2];
		if(verbose)
			fprint(2, "tempo %ud\n", MIDI.Tempo);
		break;
	case 0: case 0x20: case 0x21: case 0x58: case 0x59: case 0x7f:
		break;
	default:
		fprint(2, "unknown metaevent %ux\n", b);
	}
}

static void
Tavuja(int Track, u8int First, ulong posi, ulong Len)
{
        ulong a;

	if(verbose){
		fprint(2, "[%d]: %02ux ", Track, First);
		for(a=0; a<Len; a++)
			fprint(2, "%02ux", MIDI.Tracks[Track][a+posi]);
		fprint(2, "\n");
	}

	/* FIXME: ignore sysex completely, do it like games/midi */
	if(First == 0xff)
		readmeta(Track, MIDI.Tracks[Track]+posi, Len);
	else if(First < 0xf0){
        //ulong a;	// ← what the fuck
            a=0;
            if((First>>4) == 8) /* Note off, modify it a bit */
            {
		/* FIXME: */
                MPU_Byte(First);
                MPU_Byte(MIDI.Tracks[Track][posi]);
                MPU_Byte(0);
                Len -= 2;
            }
            else
                MPU_Byte(First);
            for(; a<Len; a++)MPU_Byte(MIDI.Tracks[Track][a+posi]);
	//fprint(2, "ev %ux\n", First >> 4);
        switch(First >> 4)
        {
            case 0x8: /* Note off */
                SetNote(
                    First&15,                   /* chn */
                    MIDI.Tracks[Track][posi+0], /* note */
                    MIDI.Tracks[Track][posi+1], /* volume */
                    MIDI.MainVol[First&15], /* mainvol */
                    snNoteOff, 127);
                break;
            case 0x9: /* Note on */
                SetNote(
                    First&15,                   /* chn */
                    MIDI.Tracks[Track][posi+0], /* note */
                    MIDI.Tracks[Track][posi+1], /* volume */
                    MIDI.MainVol[First&15], /* mainvol */
                    snNoteOn, Chan[First&15].exp);
                break;
            case 0xA: /* Key after-touch */
                SetNote(
                    First&15,                   /* chn */
                    MIDI.Tracks[Track][posi+0], /* note */
                    MIDI.Tracks[Track][posi+1], /* volume */
                    MIDI.MainVol[First&15], /* mainvol */
                    snNoteModify, Chan[First&15].exp);
                break;
            case 0xC: /* Patch change */
                MIDI.Patch[First&15] = MIDI.Tracks[Track][posi];
                break;
            case 0xD: /* Channel after-touch */
                break;
            case 0xE: /* Wheel - 0x2000 = no change */
                a = MIDI.Tracks[Track][posi+0] |
                    MIDI.Tracks[Track][posi+1] << 7;
                SetNote(First&15,
                    -1, 0,0,
			/* FIXME: these are fucked up, see below */
                    //(int)((long)a*MIDI.PitchSense[First&15]/2 - 0x2000L), Chan[First&15].exp);
                    //(short)(((long)a - 0x2000L) *(double)MIDI.PitchSense[First&15]/8192.0), Chan[First&15].exp);
                    (short)((long)a - 0x2000L), Chan[First&15].exp);
                break;
            case 0xB: /* Controller change */
                switch(MIDI.Tracks[Track][posi+0])
                {
                    case 123: /* All notes off on channel */
                        SetNote(First&15, -1,0,0, snNoteOff, 127);
                        break;
                    case 121: /* Reset vibrato and bending */
                        MIDI.PitchSense[First&15] = 2;
                        SetNote(First&15, -1,0,0, 0, Chan[First&15].exp); /* last 0=bend 0 */
                        break;
                    case 7:
                        MIDI.MainVol[First&15] = MIDI.Tracks[Track][posi+1];

			SetNote(First&15, -1, 0, MIDI.MainVol[First&15], snNoteUpdate, Chan[First&15].exp);
			/* FIXME: why doesn't this work? */
			//for(a = 0; a < 16; a++)
			//	SetNote(a, -1, 0, MIDI.MainVol[First&15], snNoteUpdate, Chan[First&15].exp);
                        break;
                    case 64:
			if(verbose)
				fprint(2, "ctl 64: sustain unsupported\n");
			break;
                    case 1:
			if(verbose)
				fprint(2, "ctl 1: vibrato unsupported\n");
			break;
                    case 91:
			if(verbose)
				fprint(2, "ctl 91 %ud: reverb depth unsupported\n", MIDI.Tracks[Track][posi+1]);
                        break;
                    case 93:
			if(verbose)
                        	fprint(2, "ctl 93 %ud: chorus depth unsupported\n", MIDI.Tracks[Track][posi+1]);
                        break;
                    case 0x06:   /* Pitch bender sensitivity */
			/* FIXME: the fuck is this? whatever it is,
			 * it's broken */
                        MIDI.PitchSense[First&15] = MIDI.Tracks[Track][posi+1];
                        break;
                    case 0x0a:  /* Pan */
                        MIDI.Pan[First&15] = MIDI.Tracks[Track][posi+1];
                        break;
                    case 0:
			/* FIXME: unimplemented: select bank */
			if(verbose)
                        	fprint(2, "ctl 0 %ud: bank select unsupported\n", MIDI.Tracks[Track][posi+1]);
                        break;
		case 0x0b:	/* FIXME: probably bullshizzles */
                        SetNote(First&15, -1,0,MIDI.MainVol[First&15], snNoteUpdate, MIDI.Tracks[Track][posi+1]);
			break;
                    default:
			if(verbose)
                        	fprint(2, "unknown ctl %ud: %ux\n",
					MIDI.Tracks[Track][posi+0],
					MIDI.Tracks[Track][posi+1]);
			}
                break;
		}
	}
}

/* Return value: 0=ok, -1=user break */
static int play(void)
{
    int a, NotFirst, Userbreak=0;
    long Viivetta;

	if((MIDI.Waiting = mallocz(MIDI.TrackCount * sizeof *MIDI.Waiting, 1)) == nil
	|| (MIDI.sWaiting = mallocz(MIDI.TrackCount * sizeof *MIDI.sWaiting, 1)) == nil
	|| (MIDI.SWaiting = mallocz(MIDI.TrackCount * sizeof *MIDI.SWaiting, 1)) == nil
	|| (MIDI.Posi = mallocz(MIDI.TrackCount * sizeof *MIDI.Posi, 1)) == nil
	|| (MIDI.sPosi = mallocz(MIDI.TrackCount * sizeof *MIDI.sPosi, 1)) == nil
	|| (MIDI.SPosi = mallocz(MIDI.TrackCount * sizeof *MIDI.SPosi, 1)) == nil
	|| (MIDI.Running = mallocz(MIDI.TrackCount * sizeof *MIDI.Running, 1)) == nil
	|| (MIDI.sRunning = mallocz(MIDI.TrackCount * sizeof *MIDI.sRunning, 1)) == nil
	|| (MIDI.SRunning = mallocz(MIDI.TrackCount * sizeof *MIDI.SRunning, 1)) == nil)
		sysfatal("mallocz: %r");

    for(a=0; a<MIDI.TrackCount; a++)
    {
        ulong c = 0;
        MIDI.sWaiting[a]= GetVarLen(a, &c);
        MIDI.sRunning[a]= 0;
        MIDI.sPosi[a]   = c;
    }

    for(a=0; a<16; a++)
    {
        MIDI.Pan[a]        = 64;   /* Middle      */
        MIDI.Patch[a]      = 1;    /* Piano       */
        MIDI.PitchSense[a] = 2;    /* � seminotes */
        MIDI.MainVol[a]    = 127;
        MIDI.Bend[a]       = 0;
    }

    NotFirst = 0;
//ReLoop:
    for(a=0; a<MIDI.TrackCount; a++)
    {
        MIDI.Posi[a]    = MIDI.sPosi[a];
        MIDI.Waiting[a] = MIDI.sWaiting[a];
        MIDI.Running[a] = MIDI.sRunning[a];
    }

    MIDI.Tempo = MIDI.initempo;
    
    memset(Chan, 0, sizeof Chan);
	for(a = 0; a < nelem(Chan); a++){
		Chan[a].exp = 0x7f;
	}

    memset(MIDI.Used,    0, sizeof MIDI.Used);

    Viivetta = 0;
 
    for(;;)
    {
        int Fin, Act;

        if(NotFirst)
        {
                long Lisa = MIDI.Tempo/MIDI.DeltaTicks;
                
                /* tempo      = microseconds per quarter note
                 * deltaticks = number of delta-time ticks per quarter note
                 *
                 * So, when tempo = 200000
                 * and deltaticks = 10,
                 * then 10 ticks have 200000 microseconds.
                 * 20 ticks have 400000 microseconds.
                 * When deltaticks = 5,
                 * then 10 ticks have 40000 microseconds.
                 */

                Viivetta += Lisa;
                if(Viivetta >= MINDELAY)
                {
                    mix(Viivetta);
                    Viivetta = 0;
                }
            AnalyzeRow();
        }
        else
        {
            u32int b = 0xFFFFFFFFUL;
            /* Find the smallest delay */
            for(a=0; a<MIDI.TrackCount; a++)
                if(MIDI.Waiting[a] < b)
                    b = MIDI.Waiting[a];
            /* Elapse that delay from all tracks */
            for(a=0; a<MIDI.TrackCount; a++)
                MIDI.Waiting[a] -= b;
        }

        /* Let the notes on channels become older (Age++) only if  *
         * something happens. This way, we don't overflow the ages *
         * too easily.                                             */
        for(Act=a=0; a<MAXS3MCHAN; a++)
            if(MIDI.Waiting[a]<=1 && MIDI.Posi[a]<MIDI.TrackLen[a])
                Act++;
        for(a=0; a<MAXS3MCHAN; a++)
            if(!Chan[a].Age||Act!=0)
                Chan[a].Age++;

        for(a=0; a<MIDI.TrackCount; ++a)
        {
            MIDI.SPosi[a]    = MIDI.Posi[a];
            MIDI.SWaiting[a] = MIDI.Waiting[a];
            MIDI.SRunning[a] = MIDI.Running[a];
        }
        for(Fin=1, a=0; a<MIDI.TrackCount; a++)
        {
            if(MIDI.Waiting[a] > 0)MIDI.Waiting[a]--;

            if(MIDI.Posi[a] < MIDI.TrackLen[a])Fin=0;

            /* While this track has events that we should have handled */
            while(MIDI.Waiting[a]<=0 && MIDI.Posi[a]<MIDI.TrackLen[a])
            {
                ulong pos;
                u8int b = MIDI.Tracks[a][MIDI.Posi[a]];
                if(b < 128)
                    b = MIDI.Running[a];
                else
                {
                    MIDI.Posi[a]++;
                    if(b < 0xF0)MIDI.Running[a] = b;
                }

                pos = MIDI.Posi[a];

		//fprint(2, "b %ux\n", b);
                if(b == 0xFF)
                {
                    int ls=0;
                    ulong len;
                    u8int typi = MIDI.Tracks[a][MIDI.Posi[a]++];
                    len = (ulong)GetVarLen(a, &MIDI.Posi[a]);
                    if(typi == 6) /* marker */
                        if(!strncmp((char *)(MIDI.Tracks[a]+MIDI.Posi[a]),
                                    "loopStart", len))
                        {
				if(verbose)
                            		fprint(2, "Found loopStart\n");
                            ls=1;
                        }
                    MIDI.Posi[a] += len;
                    if(ls)goto SaveLoopStart;
                }
                else if(b==0xF7 || b==0xF0)
                {
                    MIDI.Posi[a] += (ulong)GetVarLen(a, &MIDI.Posi[a]);
                }
                else if(b == 0xF3)MIDI.Posi[a]++;
                else if(b == 0xF2)MIDI.Posi[a]+=2;
                else if(b>=0xC0 && b<=0xDF)MIDI.Posi[a]++;
                else if(b>=0x80 && b<=0xEF)
                {
                    MIDI.Posi[a]+=2;
                    if(b>=0x90 && b<=0x9F && !NotFirst)
                    {
                        int c;
SaveLoopStart:          NotFirst=1;
                        /* Save the starting position for looping */
                        for(c=0; c<MIDI.TrackCount; c++)
                        {
                            MIDI.sPosi[c]    = MIDI.SPosi[c];
                            MIDI.sWaiting[c] = MIDI.SWaiting[c];
                            MIDI.sRunning[c] = MIDI.SRunning[c];
                        }
                        MIDI.initempo = MIDI.Tempo;
                    }
                }

                Tavuja(a, b, pos, MIDI.Posi[a]-pos);

                if(MIDI.Posi[a] < MIDI.TrackLen[a])
                    MIDI.Waiting[a] += GetVarLen(a, &MIDI.Posi[a]);
            }
        }
        if(Fin)
		break;
    }
	write(afd, abuf, ap-abuf);
    return Userbreak;
}

static void
initinst(void)
{
	int m, i;
	InternalSample *t, *p;

	m = sizeof(adl) / 14;

	if((t = calloc(m, sizeof *t)) == nil)
		sysfatal("initinst: %r");
	p = t;
	/* FIXME: .D needs to be uchar(?) */
	/* FIXME: use char adl[][14] rather than this */
	/* FIXME: find out where the values are from */
	for(i=0; i<m; i++){
		p->D[0] = adl[i * 14 + 3];
		p->D[1] = adl[i * 14 + 9];
		p->D[2] = adl[i * 14 + 4];
		p->D[3] = adl[i * 14 + 10];
		p->D[4] = adl[i * 14 + 5];
		p->D[5] = adl[i * 14 + 11];
		p->D[6] = adl[i * 14 + 6];
		p->D[7] = adl[i * 14 + 12];
		p->D[8] = adl[i * 14 + 7] & 3;
		p->D[9] = adl[i * 14 + 13] & 3;
		p->D[10]= adl[i * 14 + 8];
		mdat.Instr[i] = p++;
	}
}

/* FIXME: get8/16/32 approach is better */
/* FIXME: use bio.h */
static void
eread(int fd, void *buf, int n)
{
	if(readn(fd, buf, n) < 0)
		sysfatal("read: %r");
}

static void
readmid(char *mid)
{
	int i, fd;
	uint n;
	uchar id[4];

	fd = 0;
	if(mid != nil && (fd = open(mid, OREAD)) < 0)
		sysfatal("open: %r");

	/* FIXME: get8/16/32 better; Conv shit is also stupid */
	/* FIXME: also, don't read file into memory but use bio? */
	eread(fd, id, 4);
	if(memcmp(id, "MThd", 4) != 0)
		sysfatal("invalid header");
	eread(fd, id, 4);
	if(ConvL(id) != 6)
		sysfatal("invalid midi file");

	eread(fd, id, 2);
	MIDI.Fmt = ConvI(id);
	eread(fd, id, 2);
	MIDI.TrackCount = ConvI(id);
	eread(fd, id, 2);
	MIDI.DeltaTicks = ConvI(id);

	MIDI.TrackLen = calloc(MIDI.TrackCount, sizeof *MIDI.TrackLen);
	if(MIDI.TrackLen == nil)
		sysfatal("calloc len %ux %ux: %r", MIDI.TrackCount, sizeof *MIDI.TrackLen);
	MIDI.Tracks = calloc(MIDI.TrackCount, sizeof *MIDI.Tracks);
	if(MIDI.Tracks == nil)
		sysfatal("calloc trs %ux %ux: %r", MIDI.TrackCount, sizeof *MIDI.Tracks);

	for(i = 0; i < MIDI.TrackCount; i++){
		eread(fd, id, 4);
		if(memcmp(id, "MTrk", 4) != 0)
			sysfatal("invalid track");

		eread(fd, id, 4);
		n = ConvL(id);
		MIDI.TrackLen[i] = n;
		if((MIDI.Tracks[i] = mallocz(n, 1)) == nil)
			sysfatal("mallocz %ux: %r", n);

		eread(fd, MIDI.Tracks[i], n);
	}
	close(fd);

	MIDI.initempo = 150000;
}

static void
usage(void)
{
	print("%s [-cd] [midfile]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;

	ARGBEGIN{
	case 'c':
		afd = 1;
		break;
	case 'd':
		verbose++;
		break;
	default:
		usage();
	}ARGEND

	readmid(*argv);
	initinst();
	for(i = 0; i < NCHIPS; i++)
		chip[i] = ym3812_init(1789772*2, RATE);

	if(afd < 0 && (afd = open("/dev/audio", OWRITE)) < 0)
		sysfatal("open: %r");
	ap = abuf;
	play();

	for(i = 0; i < NCHIPS; i++)
		ym3812_shutdown(chip[i]);
	exits(nil);
}
