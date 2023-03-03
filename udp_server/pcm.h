#ifndef __NWAY_PCM_CONVERT_H
#define __NWAY_PCM_CONVERT_H

 #ifdef __cplusplus
extern "C" {
#endif
 
int ulaw2linear(const unsigned char* src, short *dst, int srcSize);
int alaw2linear(const unsigned char* src, short *dst, int srcSize);
int linear2alaw(short* src, unsigned char *dst, int srcSize);
int linear2ulaw(short* src, unsigned char *dst, int srcSize);
int ulaw2alaw(unsigned char* src, unsigned char *dst, int srcSize);
int alaw2ulaw(unsigned char* src, unsigned char *dst, int srcSize);
int pcm2linear(unsigned char* src, short *dst, int srcSize);
int linear2pcm(short* src, unsigned char *dst, int srcSize);

#ifdef __cplusplus
};
#endif

#endif

