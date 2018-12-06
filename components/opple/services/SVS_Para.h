#ifndef __SVS_PARA_H__
#define __SVS_PARA_H__

// #define PARA_ID_MAX ITEM_ID_MAX

typedef enum
{
    APS_PARA_MODULE_GENERAL=1, // MODULE 0  FOR LOG
    APS_PARA_MODULE_DAEMON,
	APS_PARA_MODULE_LAMP,
	APS_PARA_MODULE_APS_LAMP,
	APS_PARA_MODULE_APS_METER,
	APS_PARA_MODULE_APS_NBOTA,
	APS_PARA_MODULE_APS_NB,
	APS_PARA_MODULE_APS_IOT,	
	APS_PARA_MODULE_APS_WIFI,
	APS_PARA_MODULE_APS_DEV,
	APS_PARA_MODULE_APS_REBOOT,
	APS_PARA_MODULE_APS_LOC,
	APS_PARA_MODULE_APS_TIME,
	APS_PARA_MODULE_MAIN_TASK,
	APS_PARA_MODULE_APS_EXEP,
	APS_PARA_MODULE_APS_COAP,	
	APS_PARA_MODULE_APS_TEST,
}t_aps_para_module;


extern void AppParaInit(void);
extern int AppParaRead(int module,int num,unsigned char* data,unsigned int* lenMax);
extern int AppParaWrite(int module,int num,unsigned char* data,unsigned int len);
extern int AppParaReadU32(int module,int num,unsigned int* data);
extern int AppParaWriteU32(int module,int num,unsigned int data);
extern int AppParaReadU16(int module,int num,unsigned short* data);
extern int AppParaWriteU16(int module,int num,unsigned short data);
extern int AppParaReadU8(int module,int num,unsigned char* data);
extern int AppParaWriteU8(int module,int num,unsigned char data);











#endif


