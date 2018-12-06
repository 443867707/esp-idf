#ifndef __DEV_UTILITY_H__
#define __DEV_UTILITY_H__

//#include <Los_typedef.h>
#include <Includes.h>

#define    MAX    10

typedef unsigned short    INT16U;
#define CRC_SEED   0xFFFF   // ��λ��ΪԤ��ֵ��ʹ���˹��㷨����������ʱ ��Ҫ����������ʽ�������ְλ ��� �����ܵõ����ĳ�������ʽ
#define POLY16 0x1021  // ��λΪ��ʽ��д ʵ��Ϊ0x11021

typedef void (*FUNC)(unsigned char cmd, unsigned short seqNo);

typedef struct
{
	BOOL used;
	//int socket;
	unsigned char cmd;	
	unsigned short reqid;
	unsigned char retry;
	FUNC func;
}ST_REQID;


extern ST_REQID g_astRetry[];
extern u16 g_usSeqNo;

/*
#define sw16(x) \
    ((short)( \
        (((short)(x) & (short)0x00ffU) << 8) | \
        (((short)(x) & (short)0xff00U) >> 8) ))
*/        
typedef unsigned short int uint16;
#if 0
#define sw16(A)  ((((uint16)(A) & 0xff00) >> 8) | \
	(((uint16)(A) & 0x00ff) << 8))

#define sw32(x) \
((long)( \
   (((long)(x) & (long)0x000000ff) << 24) | \
   (((long)(x) & (long)0x0000ff00) << 8) | \
   (((long)(x) & (long)0x00ff0000) >> 8) | \
   (((long)(x) & (long)0xff000000) >> 24) ))
 
// ��Ϊx86�����ǵ�λ��ǰ,��Ҫ����һ�±�������ֽ�˳��
#define htons(x) sw16(x)
#define ntohs(x) sw16(x)
#define htonl(x) sw32(x)
#define ntohl(x) sw32(x)
#endif
INT16U crc16(unsigned char *buf,unsigned short length);
//char *itoa(int value, char *string, int radix);
int _atoi(const char *nptr);
int ftoa(char *str, long double num, int n);
float _atof(const char *str);
int AINPUT(/*const int socket, */const char cmd, FUNC func, const unsigned short reqid, const char retry);
int AOUTPUT(unsigned short reqid);
int ACALL(u8 idx);
int ARETRY(unsigned short reqid);
int IS_ZERO(const char * buffer, int len);
int flashIsEmpty(const char * buffer, int len);
int string_count(char * str, char * substring);
void RoadLamp_Dump(char * buf, int len);

#endif
