#ifndef __BOOT_MMT_H__
#define __BOOT_MMT_H__

/**********************************************************************************************
                                             Defines
***********************************************************************************************/



/**********************************************************************************************
                                             Typedefs
***********************************************************************************************/
typedef enum
{
	BEC_SUCCESS=0,
	BEC_CRC_ERROR,
	FLASH_READ_ERR,
	BEC_FLASH_WRITE_ERR,
	
}t_bootmmt_err_code;

typedef enum
{
    BOOT_NONE=0,
    BOOT_CP21_BOOT_AP1,
    BOOT_APP1,
    BOOT_APP2,
}t_boot_type;

typedef enum
{
    BOOT_FAIL_KEEP=0,
    BOOT_FAIL_BOOT_ANOTHER_AREA,
}t_boot_fail_option;

typedef enum
{
	BOOT_IMAGE_WITHOUT_CHECK=0,
	BOOT_IMAGE_WITH_CHECK,
}t_boot_option;

typedef enum
{
	FLASH_POS_INNER=0,
	FLASH_POS_EXTERNAL,
}t_flash_postion;

typedef struct
{
    unsigned int flash_position;
	  unsigned int boot_option;
	  unsigned int area_start;
    unsigned int area_size;
    unsigned int run_start;
    unsigned int checksum;
}t_app_info;

typedef void (*pboot)(void);

typedef struct
{
    unsigned int bootType;
    unsigned int failOption;
    t_app_info app1;
    t_app_info app2;
    unsigned int crc;
}t_boot_para;

/**********************************************************************************************
                                             Interfaces
***********************************************************************************************/

t_bootmmt_err_code readBootPara(t_boot_para* para);
t_bootmmt_err_code writeBootPara(t_boot_para* para);




































#endif
