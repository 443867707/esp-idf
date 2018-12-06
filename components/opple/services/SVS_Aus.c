/**
******************************************************************************
* @file    SVS_Aus.c
* @author  Liqi
* @version V1.0.0
* @date    2018-6-22
* @brief   
******************************************************************************

******************************************************************************
*/ 

#include "SVS_Aus.h"
#include "DEV_BootMmt.h"
#include "DEV_MemoryMmt.h"
#include "string.h"
#include "Includes.h"
#include "OPP_Debug.h"

#ifdef PLATFORM_CPU_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "nvs.h"
#include "nvs_flash.h"
#endif



static int ota_seq=-1,ota_seq_now;

#ifdef PLATFORM_CPU_ESP32
    const esp_partition_t *update_partition = NULL;
    const esp_partition_t *configured;
    const esp_partition_t *running;
    esp_ota_handle_t update_handle = 0 ;
#else
    static unsigned int ota_index=0;
    t_bootmmt_err_code boot_err;
    static t_boot_para boot_para;
    //static int time_start,time_use = 0;
    static unsigned char ota_buf[300];
#endif



void InitOppOta(void)
{
/*
  s	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if(err != ESP_OK)
    {
        DEBUG_LOG(DEBUG_MODULE_AUS,DLL_ERR,"OTA init error!");
    }
    
*/
}

/*
void OppOtaLoop(void)
{
	
}
*/
int OppOtaPrepare(void)
{
#ifdef PLATFORM_CPU_ESP32



  return 0;
#else
  t_memory_err_code err;
	err = memory_erase(MB_APP_OTA,0,memory_mmt[MB_APP_OTA].size);
	
	if(err != 0)
	{
		return 1;
	}
	return 0;
#endif
}

int OppOtaStartHandle(void)
{
#ifdef PLATFORM_CPU_ESP32
  esp_err_t err;

  //
  configured = esp_ota_get_boot_partition();
  running = esp_ota_get_running_partition();
  if (configured != running) {
        DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO,"(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO,"Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);
   
    //
    update_partition = esp_ota_get_next_update_partition(NULL);
    DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO,  "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO,  "esp_ota_begin failed (%s)", esp_err_to_name(err));
        return 1;
    }
    DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO, "esp_ota_begin succeeded");
    
  return 0;
#else
	ota_index = 0;
	ota_seq = -1;
	//time_start = OppTickCountGet();
#endif
	return 0;
}

int OppOtaReceivingHandle(unsigned short seq,const unsigned char* data,unsigned short len)
{
#ifdef PLATFORM_CPU_ESP32
    esp_err_t err;
#else
  t_memory_err_code err;
#endif
	
	ota_seq_now = seq;
	if(ota_seq_now == ota_seq) 
	{
		return 0;
	}
	if((ota_seq_now != (ota_seq+1))&&(ota_seq != -1))
	{
		return 1;
	}	
	else // ota_seq_now == (ota_seq+1) || ota_seq != -1
	{
	}
	
#ifdef PLATFORM_CPU_ESP32
	err = esp_ota_write( update_handle, (const void *)data, len);
  if (err != ESP_OK) {
      DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO,  "Error: esp_ota_write failed (%s)!", esp_err_to_name(err));
      //return 1;
  }
#else
  memcpy(ota_buf,data,len);
	err = memory_program(MB_APP_OTA,ota_index,ota_buf,len);
	ota_index += len;
#endif
	
	if(err != 0) 
	{
		return 1;
	}
	else
	{
		ota_seq = ota_seq_now;
	}
	
	return 0;
}

int OppOtaReceiveEndHandle(void)
{
#ifdef PLATFORM_CPU_ESP32
  esp_err_t err;

  if (esp_ota_end(update_handle) != ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO, "esp_ota_end failed!");
        //task_fatal_error();
        return 1;
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO,"esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        //task_fatal_error();
        return 1;
    }
    DEBUG_LOG(DEBUG_MODULE_AUS,DLL_INFO, "Prepare to restart system!");
    // esp_restart();


  return 0;
#else
	//t_memory_block block_from=MB_APP_OTA,block_to=MB_APP;
	//time_use = OppTickCountGet();
	//time_use -= time_start;

	boot_para.bootType = BOOT_CP21_BOOT_AP1;
	boot_para.failOption = BOOT_FAIL_KEEP;
	
	boot_para.app2.area_start = memory_mmt[MB_APP_OTA].addr;
	boot_para.app2.area_size = memory_mmt[MB_APP_OTA].size;
	boot_para.app2.run_start = boot_para.app2.area_start+4;
	boot_para.app2.boot_option = BOOT_IMAGE_WITHOUT_CHECK;
	boot_para.app2.flash_position = memory_mmt[MB_APP_OTA].pos;

	boot_para.app1.area_start = memory_mmt[MB_APP].addr;
	boot_para.app1.run_start = boot_para.app1.area_start+4;
	boot_para.app1.area_size = memory_mmt[MB_APP].size;
	boot_para.app1.boot_option = BOOT_IMAGE_WITHOUT_CHECK;
	boot_para.app1.flash_position = memory_mmt[MB_APP].pos;
	
	boot_err = writeBootPara(&boot_para);
	if(boot_err != 0)
	{
		boot_err = writeBootPara(&boot_para);
	}
	if(boot_err !=0)
	{
		return 1;
	}
	
	return 0;
#endif
}
