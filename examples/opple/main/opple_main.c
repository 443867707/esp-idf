/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "APP_Main.h"
#include "SVS_Udp.h"
#include "string.h"
#include <stdio.h>
#include "cli-interactor.h"
#include "cli-interpreter.h"
#include "cli.h"
#include "SVS_Ota.h"
#include "SVS_WiFi.h"
#include "OPP_Debug.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "OS.h"
#include "esp_task_wdt.h"
#include "rom/rtc.h"
#include "opple_main.h"
#include "SVS_Para.h"
#include "SVS_SafeMode.h"
#include "DEV_ItemMmt.h"



/*************************************************************************************
Defines
*************************************************************************************/
#define MAIN_OPT_CHECK(o,s) \
if((o) != 0){\
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR,"%s,err=%d\r\n",s,o);\
}

#define TASK_WDT_PERIOD_MIN_SEC 300  // 线程看门狗最小喂狗周期（S）5Min
#define TASK_WDT_PERIOD_MAX_SEC 1800  // 线程看门狗最大喂狗周期（S）30Min

#define MODULE_MAIN_NUM_WDTTASK    0
#define MODULE_MAIN_MUM_SYSMODE    1
#define MODULE_MAIN_MUM_SYSMODE_B2 2
#define MODULE_MAIN_MUM_SYSMODE_B3 3



/*************************************************************************************
Vars
*************************************************************************************/
OS_EVENTGROUP_T g_eventWifi; /* 用于WIFI相关线程WIFI连接状态同步 */
T_MUTEX g_stLampCfgMutex;

typedef enum
{
	TASK_MAIN=0,
	TASK_UDP,
	TASK_OTA,
	TASK_CLI,
	TASK_METER,
	TASK_NB,
	TASK_TELNET,
	/***************/
	TASK_MAX,
}TASK_EN;
static T_MUTEX mutexTaskWdt;
static TaskHandle_t gTask[TASK_MAX]={0};
static TaskWdtConfig_EN gTaskWdt={
	.resetEn=1,
	.period=TASK_WDT_PERIOD_MIN_SEC,
};
/*************************************************************************************
Task WDT
*************************************************************************************/
int taskWdtAddAll(void)
{
	MAIN_OPT_CHECK(esp_task_wdt_add(gTask[TASK_MAIN]),"WDT ADD MAIN fail");
	//MAIN_OPT_CHECK(esp_task_wdt_add(gTask[TASK_UDP] ),"WDT ADD DUP add fail");
	MAIN_OPT_CHECK(esp_task_wdt_add(gTask[TASK_METER] ),"WDT ADD METER add fail");
	MAIN_OPT_CHECK(esp_task_wdt_add(gTask[TASK_NB] ),"WDT ADD NB add fail");
	return 0;
}
int taskWdtDelAll(void)
{
	MAIN_OPT_CHECK(esp_task_wdt_delete(gTask[TASK_MAIN]),"WDT del MAIN fail");
	//MAIN_OPT_CHECK(esp_task_wdt_delete(gTask[TASK_UDP] ),"WDT del DUP add fail");
	MAIN_OPT_CHECK(esp_task_wdt_delete(gTask[TASK_METER] ),"WDT del METER add fail");
	MAIN_OPT_CHECK(esp_task_wdt_delete(gTask[TASK_NB] ),"WDT del NB add fail");
	return 0;
}
int taskWdtReset(void)
{
	MAIN_OPT_CHECK(esp_task_wdt_reset(),"Task wdt reset fail");
	return 0;
}
int taskWdtEnable(unsigned int timeSec,unsigned int resetEn)
{
	if((timeSec >= TASK_WDT_PERIOD_MIN_SEC)&&(timeSec<= TASK_WDT_PERIOD_MAX_SEC)){
		MAIN_OPT_CHECK(esp_task_wdt_init(timeSec, resetEn),"Task wdt init fail!");
		return 0;
	}else{
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "taskWdtEnable invalid period!\r\n");
		return 1;
	}
}
int taskWdtDisable(void)
{
	taskWdtDelAll();
	MAIN_OPT_CHECK(esp_task_wdt_deinit(),"Task wdt deinit fail!");
	return 0;
}

int taskWdtConfigSet(TaskWdtConfig_EN* config)
{
	int res;
	MUTEX_LOCK(mutexTaskWdt,MUTEX_WAIT_ALWAYS);
	if(config->period < TASK_WDT_PERIOD_MIN_SEC){
		res = 1;
	}
	else if(config->period > TASK_WDT_PERIOD_MAX_SEC){
		res = 2;
	}
	else{
		//res = AppParaWrite(APS_PARA_MODULE_MAIN_TASK,0,(unsigned char*)config,sizeof(TaskWdtConfig_EN));
		//if(res ==0)  // 调试阶段，不处理FLASH读写
		//{
			res=0;     // 调试阶段，不处理FLASH读写
			memcpy(&gTaskWdt,config,sizeof(TaskWdtConfig_EN));
			taskWdtEnable(config->period,config->resetEn);
		//}
	}
	MUTEX_UNLOCK(mutexTaskWdt);
	return res;
}
int taskWdtConfigGet(TaskWdtConfig_EN* config)
{
	MUTEX_LOCK(mutexTaskWdt,MUTEX_WAIT_ALWAYS);
	memcpy(config,&gTaskWdt,sizeof(TaskWdtConfig_EN));
	MUTEX_UNLOCK(mutexTaskWdt);
	return 0;
}
int taskWdtInit(void)
{
	MUTEX_CREATE(mutexTaskWdt);

	// 测试阶段，上电后直接使用默认值，不从FLASH中读取
	
	gTaskWdt.resetEn = 1;
	gTaskWdt.period = TASK_WDT_PERIOD_MIN_SEC;
	return 0;
	
	if(AppParaRead(APS_PARA_MODULE_MAIN_TASK,0,(unsigned char*)&gTaskWdt,sizeof(gTaskWdt))!=0)
	{
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "TaskWdtInit load default para!\r\n");
		gTaskWdt.resetEn = 1;
		gTaskWdt.period = TASK_WDT_PERIOD_MIN_SEC;
	}
	else
	{	
		if(gTaskWdt.period < TASK_WDT_PERIOD_MIN_SEC){
			gTaskWdt.period = TASK_WDT_PERIOD_MIN_SEC;
		}
		if(gTaskWdt.period >= TASK_WDT_PERIOD_MAX_SEC){
			gTaskWdt.period = TASK_WDT_PERIOD_MAX_SEC;
		}
	}
	
	return 0;
}


/*************************************************************************************
Tasks
*************************************************************************************/

void task_cli()
{
    int c;
        
    cliInit();
	while (1)
	{
	    c=fgetc(stdin);
	    if(c != EOF){
			CliIttInput(CLI_CHL_COM,(char)c);
	    }
        cliLoop();
	    vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}

int sysModeGet(U16 *mode,U16* cnt)
{
	U32 sysModeCnt1=0,sysModeCnt2=0,sysModeCnt3=0;
	int res1,res2,res3;

	if(mode == NULL || cnt == NULL)
	{
		printf("In pointer is null!\r\n");
		return 1;
	}
	
	res1 = DataItemReadU32(APS_PARA_MODULE_MAIN_TASK,MODULE_MAIN_MUM_SYSMODE,&sysModeCnt1);
	res2 = DataItemReadU32(APS_PARA_MODULE_MAIN_TASK,MODULE_MAIN_MUM_SYSMODE_B2,&sysModeCnt2);
	res3 = DataItemReadU32(APS_PARA_MODULE_MAIN_TASK,MODULE_MAIN_MUM_SYSMODE_B3,&sysModeCnt3);
    if((res1+res2+res3) != 0){
    	*mode = SYS_MODE_NORMAL;
    	*cnt = 0;
    	return (res1+res2+res3);
    }else{
    	if((sysModeCnt2==sysModeCnt1*10) && (sysModeCnt3==sysModeCnt1*60)){
	    	*mode = (U16)((sysModeCnt1 >> 16) & 0x0000FFFF);
	    	*cnt = (U16)(sysModeCnt1 & 0x0000FFFF);
	    	return 0;
    	}else{
    		*mode = SYS_MODE_NORMAL;
    		*cnt = 0;
    		return 1;
    	}
    }
}

int sysModeSet(U16 mode,U16 cnt)
{
	U32 sysModeCnt;
	int res1,res2,res3;

	sysModeCnt = mode;
	sysModeCnt <<= 16;
	sysModeCnt += cnt;

	res1 = DataItemWriteU32(APS_PARA_MODULE_MAIN_TASK,MODULE_MAIN_MUM_SYSMODE,sysModeCnt);
	res2 = DataItemWriteU32(APS_PARA_MODULE_MAIN_TASK,MODULE_MAIN_MUM_SYSMODE_B2,sysModeCnt*10);
	res3 = DataItemWriteU32(APS_PARA_MODULE_MAIN_TASK,MODULE_MAIN_MUM_SYSMODE_B3,sysModeCnt*60);

	return (res1+res2+res3);
}

int sysModeDecreaseSet(void)
{
	U16 mode,cnt;
	int res;

	res = sysModeGet(&mode,&cnt);
	if(res != 0) return res;

	if(cnt>0)
	{
		cnt=cnt-1;
		return sysModeSet(mode,cnt);
	}
	else
	{
		// Do nothing
	}
	
	return 0;
}

void app_main()
{
	int rst_reas[2]; // RESET_REASON
	unsigned int sysModeCnt = 0,mode=0,cnt=0;
	int res;

	/******************************************************************************************************
    Init setp 0
    ******************************************************************************************************/
    rst_reas[0] = rtc_get_reset_reason(0);
    rst_reas[1] = rtc_get_reset_reason(1);
    
    printf("System Restart,reset reason:cpu0(%d),cpu1(%d)!\r\n",rst_reas[0],rst_reas[1]);
    
	//g_eventWifi = xEventGroupCreate();
	OS_EVENTGROUP_CREATE(g_eventWifi);
	/*Init nvs*/
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

	/******************************************************************************************************/
	/* sys mode                                                                                          */
	/*                                                                                                    */
	/******************************************************************************************************/
    res = sysModeGet(&mode,&cnt);
    if(res != 0){
    	printf("Load sysmode para fail!\r\n");
    	mode = SYS_MODE_NORMAL;
    	cnt = 0;
    }else{
    	printf("Load sysmode para success,mode=%u,cnt=%u!\r\n",mode,cnt);
    }
	
    if(mode == SYS_MODE_NORMAL)
    {
    	cnt++;
		if(cnt >= SYS_MODE_CNT_MAX){
			cnt = 0;
			mode = SYS_MODE_SAFE;
		}
		else
		{
			if(sysModeSet(mode,cnt)!=0)
			{
				printf("Set mode=%d,cnt=%d fail,try again!\r\n",mode,cnt);
				if(sysModeSet(mode,cnt)!=0){
					printf("Set mode=%d,cnt=%d fail again!\r\n",mode,cnt);
				}else{
					printf("Try again Setting mode=%d,cnt=%d success!\r\n",mode,cnt);
				}
			}else{
				printf("Set mode=%d,cnt=%d success!\r\n",mode,cnt);
			}
		}
    }

	if(mode == SYS_MODE_SAFE)
	{
		// Safe mode
		// esp_task_wdt_init(timeSec, resetEn)
		printf("Safemode:start!\r\n");
		if(sysModeSet(SYS_MODE_NORMAL,0)!=0)
		{
			printf("safemode:set mode=normal fail,try again!\r\n");
			if(sysModeSet(SYS_MODE_NORMAL,0)!=0){
				printf("safemode:set mode=normal fail again!\r\n");
			}else{
				printf("safemode:Try again setting mode=normal success!\r\n");
			}
		}
		else{
			printf("safemode:set mode=normal success!\r\n");
		}
		safe_mode_initialise_wifi();
		xTaskCreate(&safe_mode_main,"safemode_main",(1024 * 8), NULL, 10, &(gTask[TASK_MAIN]));
		xTaskCreate(&safe_mode_udp, "safemode_udp", (1024 * 8), NULL, 10, &(gTask[TASK_UDP]));
    	xTaskCreate(&safe_mode_ota, "safemode_ota", (1024 * 8), NULL, 11, &(gTask[TASK_OTA]));
    	printf("Safemode:create tasks finished!\r\n");
		return;
	}
	else
	{
		// Normal mode
		// Continue
	}
	/******************************************************************************************************/
	/* End of sys mode                                                                                   */
	/*                                                                                                    */
	/******************************************************************************************************/

    /*Init var*/
	printf("Main start init tick %d!\r\n", OppTickCountGet());
	OS_EVENTGROUP_CLEARBIT(g_eventWifi,1);
	MUTEX_CREATE(g_stLampCfgMutex);
	MainInit();
	printf("Main init compeletely tick %d!\r\n", OppTickCountGet());

   /******************************************************************************************************
   Init setp 1
   ******************************************************************************************************/
	
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "This is ESP32 chip with %d CPU cores, WiFi%s%s, \r\n",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "silicon revision %d, \r\n", chip_info.revision);
    DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "%dMB %s flash\r\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "app_main start!\r\n");

	/*Config task wdt*/
	taskWdtInit();
	taskWdtEnable(gTaskWdt.period, gTaskWdt.resetEn);
	//MAIN_OPT_CHECK(esp_task_wdt_init(25, true),"Task wdt init fail!");
	

	/*Create tasks*/
	xTaskCreate(&MainThread, "main_thread", (1024 * 8), NULL, 10, &(gTask[TASK_MAIN]));
    xTaskCreate(&UdpThread, "udp_thread", (1024 * 8), NULL, 10, &(gTask[TASK_UDP]));
    xTaskCreate(&OtaThread, "ota_thread", (1024 * 8), NULL, 11, &(gTask[TASK_OTA]));
    xTaskCreate(&task_cli, "task_cli", (1024 * 5), NULL, 9, &(gTask[TASK_CLI]));
    xTaskCreate(&ElecThread, "meter_thread", (1024 * 5), NULL, 8, &(gTask[TASK_METER]));
	xTaskCreate(&NeulBc28Thread, "nbiot_thread", (1024 * 12), NULL, 12, &(gTask[TASK_NB]));
	xTaskCreate(&telnetTask, "telnetTask", (1024 * 8), NULL, 11, &(gTask[TASK_TELNET]));

	// Add task to wdt
	taskWdtAddAll();
}



