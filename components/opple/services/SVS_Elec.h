#ifndef _SVS_ELEC_H
#define _SVS_ELEC_H

#include "APP_LampCtrl.h"

#define ELEC_ID    0
#define MAX_INNER_EC    2147483647
#define MAX_EC          178956970
#define ELEC_SCAN_TO    30000
#define ELEC_SAVE_TO    30000
#define EC_SCAN_TO      30000
#define EC_SAVE_TO      30000
#define RUNTIME_SCAN_TO      60000
#define RUNTIME_SAVE_TO      600000
#define MAX_ENERGY_REG_VALUE 65535


#pragma pack(1)
typedef struct{
	U32 hisConsumption; /*历史功耗*/
	U32 hisTime;
}ST_ELEC_CONFIG;

typedef struct
{
	U32 current;
	U32 voltage;
	U32 power;
	U32 consumption;
	U32 hisConsumption;	
	U32 delta; //改变的电量
	U32 factor;
}ST_OPP_LAMP_CURR_ELECTRIC_INFO;

#pragma pack()

int ElecInit(void);
void ElecTimerStart();
void ElecThread(void *pvParameter);
int ElecSetElectricInfo(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo);
int ElecGetElectricInfo(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo);
int ElecGetElectricInfoNow(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo);
int ElecWriteFlash(ST_ELEC_CONFIG * pstElecInfo);
int ElecReadFlash(ST_ELEC_CONFIG * pstElecInfo);
int ElecVoltageGet(int target, unsigned int * voltage);
int ElecVoltageSet(int target, int voltage);
int ElecCurrentGet(int target, unsigned int * current);
int ElecCurrentSet(int target, int current);
int ElecConsumptionSet(unsigned int consumption);
int ElecConsumptionSetInner(U32 csp);
U32 ElecHisConsumptionGetInner(void);
U32 ElecConsumptionGetInner(void);
int ElecHisConsumptionSetInner(U32 csp);
int ElecHisConsumptionSet(unsigned int hisConsumption);
int ElecSetElectricInfoCustom(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo);
int ElecDoReboot(void);

#endif
