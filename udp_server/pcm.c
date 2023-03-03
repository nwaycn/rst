//#include "stdafx.h"
#include "pcm.h"

int ss[49] = {  16,  17,  19,  21,  23,  25,  28,  31,
				34,  37,  41,  45,  50,  55,  60,  66,
				73,  80,  88,  97, 107, 118, 130, 143,
			   157, 173, 190, 209, 230, 253, 279, 307,
			   337, 371, 408, 449, 494, 544, 598, 652,
			   734, 796, 876, 963,1060,1166,1282,1411,
			  1552 };

int ml[8] = { -1,-1,-1,-1,2,4,6,8 };

#define SIGN_BIT    (0x80)      /* Sign bit for a A-law byte. */  

#define QUANT_MASK  (0xf)       /* Quantization field mask.   */  

#define NSEGS       (8)         /* Number of A-law segments.  */  

#define SEG_SHIFT   (4)         /* Left shift for segment number. */  

#define SEG_MASK    (0x70)      /* Segment field mask. */  

  

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,  

                           0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};  

 
static unsigned char ALawCompressTable[] =
{ 
    1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 
}; 

static short ALawDecompressTable[] = 
{
    0xEA80, 0xEB80, 0xE880, 0xE980, 0xEE80, 0xEF80, 0xEC80, 0xED80, 
    0xE280, 0xE380, 0xE080, 0xE180, 0xE680, 0xE780, 0xE480, 0xE580, 
    0xF540, 0xF5C0, 0xF440, 0xF4C0, 0xF740, 0xF7C0, 0xF640, 0xF6C0, 
    0xF140, 0xF1C0, 0xF040, 0xF0C0, 0xF340, 0xF3C0, 0xF240, 0xF2C0, 
    0xAA00, 0xAE00, 0xA200, 0xA600, 0xBA00, 0xBE00, 0xB200, 0xB600, 
    0x8A00, 0x8E00, 0x8200, 0x8600, 0x9A00, 0x9E00, 0x9200, 0x9600, 
    0xD500, 0xD700, 0xD100, 0xD300, 0xDD00, 0xDF00, 0xD900, 0xDB00, 
    0xC500, 0xC700, 0xC100, 0xC300, 0xCD00, 0xCF00, 0xC900, 0xCB00, 
    0xFEA8, 0xFEB8, 0xFE88, 0xFE98, 0xFEE8, 0xFEF8, 0xFEC8, 0xFED8, 
    0xFE28, 0xFE38, 0xFE08, 0xFE18, 0xFE68, 0xFE78, 0xFE48, 0xFE58, 
    0xFFA8, 0xFFB8, 0xFF88, 0xFF98, 0xFFE8, 0xFFF8, 0xFFC8, 0xFFD8, 
    0xFF28, 0xFF38, 0xFF08, 0xFF18, 0xFF68, 0xFF78, 0xFF48, 0xFF58, 
    0xFAA0, 0xFAE0, 0xFA20, 0xFA60, 0xFBA0, 0xFBE0, 0xFB20, 0xFB60, 
    0xF8A0, 0xF8E0, 0xF820, 0xF860, 0xF9A0, 0xF9E0, 0xF920, 0xF960, 
    0xFD50, 0xFD70, 0xFD10, 0xFD30, 0xFDD0, 0xFDF0, 0xFD90, 0xFDB0, 
    0xFC50, 0xFC70, 0xFC10, 0xFC30, 0xFCD0, 0xFCF0, 0xFC90, 0xFCB0, 
    0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280, 
    0x1D80, 0x1C80, 0x1F80, 0x1E80, 0x1980, 0x1880, 0x1B80, 0x1A80, 
    0x0AC0, 0x0A40, 0x0BC0, 0x0B40, 0x08C0, 0x0840, 0x09C0, 0x0940, 
    0x0EC0, 0x0E40, 0x0FC0, 0x0F40, 0x0CC0, 0x0C40, 0x0DC0, 0x0D40, 
    0x5600, 0x5200, 0x5E00, 0x5A00, 0x4600, 0x4200, 0x4E00, 0x4A00, 
    0x7600, 0x7200, 0x7E00, 0x7A00, 0x6600, 0x6200, 0x6E00, 0x6A00, 
    0x2B00, 0x2900, 0x2F00, 0x2D00, 0x2300, 0x2100, 0x2700, 0x2500, 
    0x3B00, 0x3900, 0x3F00, 0x3D00, 0x3300, 0x3100, 0x3700, 0x3500, 
    0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128, 
    0x01D8, 0x01C8, 0x01F8, 0x01E8, 0x0198, 0x0188, 0x01B8, 0x01A8, 
    0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028, 
    0x00D8, 0x00C8, 0x00F8, 0x00E8, 0x0098, 0x0088, 0x00B8, 0x00A8, 
    0x0560, 0x0520, 0x05E0, 0x05A0, 0x0460, 0x0420, 0x04E0, 0x04A0, 
    0x0760, 0x0720, 0x07E0, 0x07A0, 0x0660, 0x0620, 0x06E0, 0x06A0, 
    0x02B0, 0x0290, 0x02F0, 0x02D0, 0x0230, 0x0210, 0x0270, 0x0250, 
    0x03B0, 0x0390, 0x03F0, 0x03D0, 0x0330, 0x0310, 0x0370, 0x0350,
};

static unsigned char MuLawCompressTable[] = 
{
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 
};

static short MuLawDecompressTable[] = 
{ 
    0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84, 
    0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84, 
    0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84, 
    0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84, 
    0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804, 
    0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004, 
    0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444, 
    0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844, 
    0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64, 
    0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64, 
    0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74, 
    0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74, 
    0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC, 
    0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C, 
    0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0, 
    0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000, 
    0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C, 
    0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C, 
    0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C, 
    0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C, 
    0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC, 
    0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC, 
    0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC, 
    0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC, 
    0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C, 
    0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C, 
    0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C, 
    0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C, 
    0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104, 
    0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084, 
    0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040, 
    0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000
};
 

/* copy from CCITT G.711 specifications */  

unsigned char _u2a[128] = { /* u- to A-law conversions */  

    1,  1,  2,  2,  3,  3,  4,  4,  

    5,  5,  6,  6,  7,  7,  8,  8,  

    9,  10, 11, 12, 13, 14, 15, 16,  

    17, 18, 19, 20, 21, 22, 23, 24,  

    25, 27, 29, 31, 33, 34, 35, 36,  

    37, 38, 39, 40, 41, 42, 43, 44,  

    46, 48, 49, 50, 51, 52, 53, 54,  

    55, 56, 57, 58, 59, 60, 61, 62,  

    64, 65, 66, 67, 68, 69, 70, 71,  

    72, 73, 74, 75, 76, 77, 78, 79,  

    81, 82, 83, 84, 85, 86, 87, 88,  

    89, 90, 91, 92, 93, 94, 95, 96,  

    97, 98, 99, 100,101,102,103,104,  

    105,106,107,108,109,110,111,112,  

    113,114,115,116,117,118,119,120,  

    121,122,123,124,125,126,127,128  

};  

  

unsigned char _a2u[128] = { /* A- to u-law conversions */  

    1,  3,  5,  7,  9,  11, 13, 15,  

    16, 17, 18, 19, 20, 21, 22, 23,  

    24, 25, 26, 27, 28, 29, 30, 31,  

    32, 32, 33, 33, 34, 34, 35, 35,  

    36, 37, 38, 39, 40, 41, 42, 43,  

    44, 45, 46, 47, 48, 48, 49, 49,  

    50, 51, 52, 53, 54, 55, 56, 57,  

    58, 59, 60, 61, 62, 63, 64, 64,  

    65, 66, 67, 68, 69, 70, 71, 72,  

    73, 74, 75, 76, 77, 78, 79, 79,  

    80, 81, 82, 83, 84, 85, 86, 87,  

    88, 89, 90, 91, 92, 93, 94, 95,  

    96, 97, 98, 99, 100,101,102,103,  

    104,105,106,107,108,109,110,111,  

    112,113,114,115,116,117,118,119,  

    120,121,122,123,124,125,126,127  

};  


  

static int search(int val,short *table,int size)  

{  

    int     i;  

    for (i = 0; i < size; i++) {  

        if (val <= *table++)  

            return (i);  

    }  

    return (size);  

}  

  

/********************************************************************* 

 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law 

 *   

 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data. 

 * 

 *  Linear Input Code       Compressed Code 

 *  -----------------       ------------------ 

 *  0000000wxyza            000wxyz 

 *  0000001wxyza            001wxyz 

 *  000001wxyzab            010wxyz 

 *  00001wxyzabc            011wxyz 

 *  0001wxyzabcd            100wxyz 

 *  001wxyzabcde            101wxyz 

 *  01wxyzabcdef            110wxyz 

 *  1wxyzabcdefg            111wxyz 

 * 

 * For further information see John C. Bellamy's Digital Telephony, 1982, 

 * John Wiley & Sons, pps 98-111 and 472-476. 

 *********************************************************************/  

unsigned char lineartoalaw(short pcm_val)  /* 2's complement (16-bit range) */  

{  

    int             mask;  

    int             seg;  

    unsigned char   aval;  

  

    if (pcm_val >= 0) {  

        mask = 0xD5;        /* sign (7th) bit = 1 */  

    } else {  

        mask = 0x55;        /* sign bit = 0 */  

        pcm_val = -pcm_val - 8;  

    }  

  

    /* Convert the scaled magnitude to segment number. */  

    seg = search(pcm_val, seg_end, 8);  

  

    /* Combine the sign, segment, and quantization bits. */  

  

    if (seg >= 8)        /* out of range, return maximum value. */  

        return (0x7F ^ mask);  

    else {  

        aval = seg << SEG_SHIFT;  

        if (seg < 2)  

            aval |= (pcm_val >> 4) & QUANT_MASK;  

        else  

            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;  

        return (aval ^ mask);  

    }  

}  

  

/********************************************************************* 

 *    alaw2linear() - Convert an A-law value to 16-bit linear PCM 

 *********************************************************************/  

short alawtolinear(unsigned char a_val)  

{  

    int     t;  

    int     seg;  

  

    a_val ^= 0x55;  

  

    t = (a_val & QUANT_MASK) << 4;  

    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;  

    switch (seg) {  

    case 0:  

        t += 8;  

        break;  

    case 1:  

        t += 0x108;  

        break;  

    default:  

        t += 0x108;  

        t <<= seg - 1;  

    }  

    return ((a_val & SIGN_BIT) ? t : -t);  

}  

  

#define BIAS        (0x84)      /* Bias for linear code. */  

  

/********************************************************************* 

 * linear2ulaw() - Convert a linear PCM value to u-law 

 * 

 * In order to simplify the encoding process, the original linear magnitude 

 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to 

 * (33 - 8191). The result can be seen in the following encoding table: 

 * 

 *  Biased Linear Input Code    Compressed Code 

 *  ------------------------    --------------- 

 *  00000001wxyza               000wxyz 

 *  0000001wxyzab               001wxyz 

 *  000001wxyzabc               010wxyz 

 *  00001wxyzabcd               011wxyz 

 *  0001wxyzabcde               100wxyz 

 *  001wxyzabcdef               101wxyz 

 *  01wxyzabcdefg               110wxyz 

 *  1wxyzabcdefgh               111wxyz 

 * 

 * Each biased linear code has a leading 1 which identifies the segment 

 * number. The value of the segment number is equal to 7 minus the number 

 * of leading 0's. The quantization interval is directly available as the 

 * four bits wxyz.  * The trailing bits (a - h) are ignored. 

 * 

 * Ordinarily the complement of the resulting code word is used for 

 * transmission, and so the code word is complemented before it is returned. 

 * 

 * For further information see John C. Bellamy's Digital Telephony, 1982, 

 * John Wiley & Sons, pps 98-111 and 472-476. 

 *********************************************************************/  

unsigned char lineartoulaw(short pcm_val)  /* 2's complement (16-bit range) */  

{  

    int     mask;  

    int     seg;  

    unsigned char   uval;  

  

    /* Get the sign and the magnitude of the value. */  

    if (pcm_val < 0) {  

        pcm_val = BIAS - pcm_val;  

        mask = 0x7F;  

    } else {  

        pcm_val += BIAS;  

        mask = 0xFF;  

    }  

  

    /* Convert the scaled magnitude to segment number. */  

    seg = search(pcm_val, seg_end, 8);  

  

    /* 

     * Combine the sign, segment, quantization bits; 

     * and complement the code word. 

     */  

    if (seg >= 8)        /* out of range, return maximum value. */  

        return (0x7F ^ mask);  

    else {  

        uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);  

        return (uval ^ mask);  

    }  

  

}  

  

/********************************************************************* 

 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM 

 * 

 * First, a biased linear code is derived from the code word. An unbiased 

 * output can then be obtained by subtracting 33 from the biased code. 

 * 

 * Note that this function expects to be passed the complement of the 

 * original code word. This is in keeping with ISDN conventions. 

 *********************************************************************/  

short ulawtolinear( unsigned char  u_val)  

{  

    int     t;  

  

    /* Complement to obtain normal u-law value. */  

    u_val = ~u_val;  

  

    /* 

     * Extract and bias the quantization bits. Then 

     * shift up by the segment number and subtract out the bias. 

     */  

    t = ((u_val & QUANT_MASK) << 3) + BIAS;  

    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;  

  

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));  

}  

  

/* A-law to u-law conversion */  

unsigned char alawtoulaw(unsigned char   aval)  

{  

    aval &= 0xff;  

    return ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :  

        (0x7F ^ _a2u[aval ^ 0x55]));  

}  

  

/* u-law to A-law conversion */  

unsigned char ulawtoalaw(unsigned char   uval)  

{  

    uval &= 0xff;  

    return ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)):  

           (0x55 ^ (_u2a[0x7F ^ uval] - 1)));  

}  


unsigned char LinearToAlawSample(short sample)
{
    int sign   = 0;
    int exponent = 0;
    int mantissa = 0;
    unsigned char compressedByte = 0;

    sign = ((~sample) >> 8) & 0x80;
    if (sign == 0)
    {
        sample = (short)-sample;
    }
    if (sample > 0x7F7B)
    {
        sample = 0x7F7B;
    }
    if (sample >= 0x100)
    {
        exponent = (int)ALawCompressTable[(sample >> 8) & 0x7F];
        mantissa = (sample >> (exponent + 3)) & 0x0F;
        compressedByte = (unsigned char)((exponent << 4) | mantissa);
    }
    else
    {
        compressedByte = (unsigned char)(sample >> 4);
    }
    compressedByte ^= (unsigned char)(sign ^ 0x55);
    return compressedByte;
}

short AlawToLinearSample(unsigned char sample)
{
    return ALawDecompressTable[sample];
}

unsigned char LinearToMuLawSample(short sample)
{
    int cBias = 0x84;
    int cClip = 0x7F7B;
    int sign = (sample >> 8) & 0x80;

    if (sign != 0) 
    {
        sample = (short)-sample;
    }
    if (sample > cClip) 
    {
        sample = (short)cClip;
    }
    sample = (short)(sample + cBias);

    int exponent = (int)MuLawCompressTable[(sample >> 7) & 0xFF];
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    int compressedByte = ~(sign | (exponent << 4) | mantissa);
    return (unsigned char)compressedByte;
}

short MuLawToLinearSample(unsigned char sample)
{
    return MuLawDecompressTable[sample];
}

char adpcm_compress(int SampleData, int *x1, int *index)
{
	char adpcm = 0;
	int  edn, ddn, x2;

	edn = SampleData - *x1;
	if ( edn < 0 ){
		adpcm |= 8;
		edn = - edn;
	}
	if ( edn >= ss[ *index ] ){
		adpcm |= 4;
		edn -= ss[ *index ];
	}
	if ( edn >= ss[ *index ] / 2 ){
		adpcm |= 2;
		edn -= ss[ *index ] / 2;
	}
	if ( edn >= ss[ *index ] / 4 ){
		adpcm |= 1;
		edn -= ss[ *index ] / 4;
	}

	ddn = ss[*index] /8;
	if ( 4 & adpcm )
		ddn += ss[*index];
	if ( 2 & adpcm )
		ddn += ss[*index] / 2;
	if ( 1 & adpcm )
		ddn += ss[*index] / 4;
	if ( adpcm & 8 )
		ddn = -ddn;
	x2 = *x1 + ddn;
	*x1 = x2;
	*index += ml[ adpcm & 7 ];
	if ( *index < 0 )
		*index = 0;
	if ( *index > 48 )
		*index = 48;
	return adpcm;
}


int adpcm_expand(char adpcm, int *x1, int *index)
{
	int ddn, x2;

	ddn = ss[*index] /8;
	if ( 4 & adpcm )
		ddn += ss[*index];
	if ( 2 & adpcm )
		ddn += ss[*index] / 2;
	if ( 1 & adpcm )
		ddn += ss[*index] / 4;
	if ( adpcm & 8 )
		ddn = -ddn;
	x2 = *x1 + ddn;
	*x1 = x2;
	*index += ml[ adpcm & 7 ];
	if (*index < 0 )
		*index = 0;
	if ( *index > 48 )
		*index = 48;
	return( x2 );
}



int ulaw2linear(const unsigned char* src, short *dst, int srcSize)
{
	unsigned char* end = src+srcSize;   
    while(src<end)   
        *dst++ = ulawtolinear(*src++);   
    return srcSize<<1;  
}

int alaw2linear(const unsigned char* src, short *dst, int srcSize)
{
	unsigned char* end = src+srcSize;   
    while(src<end)   
        *dst++ = alawtolinear(*src++);   
    return srcSize<<1; 
}

int linear2alaw(short* src, unsigned char *dst, int srcSize)
{
	unsigned char* end = dst+srcSize/2;   
    while(dst<end)   
        *dst++ = lineartoalaw(*src++);   
    return srcSize/2;  
}

int linear2ulaw(short* src, unsigned char *dst, int srcSize)
{
	unsigned char* end = dst+srcSize/2;   
    while(dst<end)   
        *dst++ = lineartoulaw(*src++);   
    return srcSize/2; 
}

int ulaw2alaw(unsigned char* src, unsigned char *dst, int srcSize)
{
	unsigned char* end = dst+srcSize;   
    while(dst<end)   
        *dst++ = ulawtoalaw(*src++);   
    return srcSize;  
}

int alaw2ulaw(unsigned char* src, unsigned char *dst, int srcSize)
{
	unsigned char* end = dst+srcSize;   
    while(dst<end)   
        *dst++ = alawtoulaw(*src++);   
    return srcSize; 
}

int pcm2linear(unsigned char* src, short *dst, int srcSize)
{
	short* end = dst+srcSize;   
    while(dst<end)   
        *dst++ = (*src++-128)<<8;
    return srcSize<<1; 
}

int linear2pcm(short* src, unsigned char *dst, int srcSize)
{
	unsigned char* end = dst+srcSize/2;   
    while(dst<end)   
        *dst++ = (*src++>>8)+128;
    return srcSize/2; 
}
