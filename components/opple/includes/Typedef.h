#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#include "Config.h"

#define VOID  void
#define INVALID_U8_VAL       (0xff)
#define INVALID_U16_VAL      (0xffff)
#define INVALID_U32_VAL      (0xffffffff)


#define VALID_VALUE          (1)
#define IS_VALID_VALUE(a)    (VALID_VALUE == (a))
#define IS_INVALID_VALUE(a)  ((0 == (a)) || (INVALID_U8_VAL == (a)) \
    || (INVALID_U16_VAL == (a)) || (INVALID_U32_VAL == (a)))

#define IOT_MIN_VAL(a, b)    (((a) > (b)) ? (b) : (a))
#define IOT_MAX_VAL(a, b)    (((a) < (b)) ? (b) : (a))

/*字节序转换宏定义*/
#if 1
/*主机序(小端序)和网络序(大端序)相反*/

//For Linux ?????
#if 0

#define OPP_NTOHL(x) ntohl((x))

#define OPP_NTOHS(x) ntohs((x))

#else

#define OPP_NTOHL(x)  ((((x) & 0xff000000) >> 24) \
    | (((x) & 0x00ff0000) >> 8) \
    | (((x) & 0x0000ff00) << 8) \
    | (((x) & 0x000000ff) << 24))

#define OPP_NTOHS(x)  ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))

#endif

#else
/*主机序(大端序)和网络序(大端序)一致*/

#define OPP_NTOHL(x)  (x)

#define OPP_NTOHS(x)  (x)

#endif
#define OPP_HTONL(x)  (OPP_NTOHL((x)))
#define OPP_HTONS(x)  (OPP_NTOHS((x)))

#define OPP_SWAP_L(x)  do {\
    x = OPP_NTOHL((x));\
} while(0)

#define OPP_SWAP_S(x)  do {\
    x = OPP_NTOHS((x));\
} while(0)

#if defined(PLATFORM_CPU_STM32)||defined(PLATFORM_CPU_ESP32)
#ifndef U8
#define U8 unsigned char
    //typedef unsigned char    U8;
#endif

#ifndef S8
#define S8 signed   char
    //typedef signed   char    S8;
#endif

#ifndef U16
#define U16 unsigned short
    //typedef unsigned short   U16;
#endif

#ifndef S16
#define S16 signed   short
    //typedef signed   short   S16;
#endif

#ifndef U32
#define U32 unsigned int
    //typedef unsigned long    U32;
#endif

#ifndef S32
#define S32 signed   int
    //typedef signed   long    S32;
#endif

#ifndef U64
#define U64 unsigned long long
	//typedef unsigned long long    U64;
#endif
	
#ifndef u8
#define u8 unsigned char
    //typedef unsigned char    u8;
#endif

#ifndef s8
#define s8  signed   char
    //typedef signed   char    s8;
#endif

#ifndef u16
#define u16  unsigned short
    //typedef unsigned short   u16;
#endif

#ifndef s16
#define s16 signed   short
    //typedef signed   short   s16;
#endif

#ifndef u32
#define u32 unsigned long
    //typedef unsigned long    u32;
#endif

#ifndef s32
#define s32 signed long
    //typedef signed   long    s32;
#endif

#ifndef u64
#define u64 unsigned long long
	//typedef unsigned long long    u64;
#endif

#ifndef UINT8
#define UINT8 unsigned char
    //typedef unsigned char    UINT8;
#endif

#ifndef INT8
#define INT8 signed   char
    //typedef signed   char    INT8;
#endif

#ifndef UINT16
#define UINT16 unsigned short
    //typedef unsigned short   UINT16;
#endif

#ifndef INT16
#define INT16 signed   short
    //typedef signed   short   INT16;
#endif

#ifndef UINT32
#define UINT32 unsigned long
    //typedef unsigned long    UINT32;
#endif

#ifndef INT32
#define INT32 signed   long
    //typedef signed   long    INT32;
#endif

#ifndef UINT64
#define UINT64 unsigned long long
	//typedef unsigned long long    UINT64;
#endif
	/*typedef unsigned long    uint8_t;
	typedef unsigned long    uint16_t;
	typedef unsigned long    uint32_t;
	typedef unsigned long    int8_t;
	typedef unsigned long    int16_t;
	typedef unsigned long    int32_t;*/
    typedef float            FLOAT;
    typedef double           DOUBLE;
	typedef enum {FALSE = 0,TRUE = 1} BOOL;
#else
    typedef unsigned char    U8;
    typedef signed   char    S8;
    typedef unsigned short   U16;
    typedef signed   short   S16;
    typedef unsigned long    U32;
    typedef signed   long    S32;
		typedef unsigned long long    U64;
    typedef float            FLOAT;
    typedef double           DOUBLE;
#endif

/*
typedef enum
{
    FALSE =0,
    TRUE =1,
}OppBool_t;

typedef enum
{
    DISABLE=0,
    ENABLE = 1,
}OppEn_t;

typedef enum
{
   SUCCESS=0,
   FAIL=1,
}OppErr_t;
*/

#endif
