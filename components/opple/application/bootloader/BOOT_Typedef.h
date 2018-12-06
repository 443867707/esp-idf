#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__



typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;
typedef signed char    INT8;
typedef signed short   INT16;
typedef signed int     INT32;


typedef enum
{
    BEC_SUCCESS=0U,
    BEC_LEN_ERR,
    BEC_FAIL,
    BEC_ERASE_FAIL,
    BEC_PROGRAM_FAIL,
    BEC_VERIFY_FAIL,
	  BEC_BOOT_FAIL,
}t_boot_err_code;

#endif


