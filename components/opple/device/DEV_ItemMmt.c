/*****************************************************************
                              How it works
******************************************************************/
/*



*/
/*****************************************************************
                              Enable
******************************************************************/
#define PLATFORM_STM32 1
#define PLATFORM_ESP32 2
#define PLATFORM_DEFINE PLATFORM_ESP32


/*****************************************************************
                              Includes
******************************************************************/
#include "Includes.h"
#include "DEV_ItemMmt.h"
#include <stdio.h>

#if PLATFORM_DEFINE==PLATFORM_ESP32
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#endif

/*****************************************************************
                              Defines
******************************************************************/
#define APS_PARA_KEY_NUM 6


/*****************************************************************
                              Typedefs
******************************************************************/




/*****************************************************************
                              Funs
******************************************************************/
inline void AppKeyGen(int module,int num,char* key)
{
    // "Pm[module]n[num]"
    sprintf(key,"m%xn%x",module,num);
}



/* return 0:success,1:fail*/
int DataItemRead(U32 module,U32 id,unsigned char* data,unsigned int* lenMax)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32    
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;
    
    AppKeyGen(module,id,key);

    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}
    
    // Read
    //err = nvs_get_blob(my_handle, key, data, &lenMax);
    err = nvs_get_blob(my_handle, key, data, lenMax);
    // if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    if (err != ESP_OK) {nvs_close(my_handle); return err;}
    
    // Close
    nvs_close(my_handle);
#else
	
#endif
    return 0;
}

/*return 0:success,1:fail*/
int DataItemWrite(U32 module,U32 id,unsigned char* data,unsigned int len)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32 
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;
    
    // Generate key
    AppKeyGen(module,id,key);

    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}
    
    // Write key-value
    err = nvs_set_blob(my_handle,key, data, len);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else

#endif
    return 0;
}

int DataItemReadU32(U32 module,U32 id,unsigned int* data)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32 
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;
    
    // Generate key
    AppKeyGen(module,id,key);

    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}

    // Read
    err = nvs_get_u32(my_handle, key, data);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else
	
#endif
    return 0;
}

int DataItemWriteU32(U32 module,U32 id,unsigned int data)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32     
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;

    // Generate key
    AppKeyGen(module,id,key);
    
    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}
    
    // Write
    err = nvs_set_u32(my_handle, key, data);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}
    
    // Commmit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else

#endif
    return 0;
}

int DataItemReadU16(U32 module,U32 id,unsigned short* data)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32 
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;
    
    // Generate key
    AppKeyGen(module,id,key);

    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}

    // Read
    err = nvs_get_u16(my_handle, key, data);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else
	
#endif
    return 0;
}

int DataItemWriteU16(U32 module,U32 id,unsigned short data)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32     
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;

    // Generate key
    AppKeyGen(module,id,key);
    
    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}
    
    // Write
    err = nvs_set_u16(my_handle, key, data);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}
    
    // Commmit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else

#endif
    return 0;
}

int DataItemReadU8(U32 module,U32 id,unsigned char* data)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32 
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;
    
    // Generate key
    AppKeyGen(module,id,key);

    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}

    // Read
    err = nvs_get_u8(my_handle, key, data);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else
	
#endif
    return 0;
}

int DataItemWriteU8(U32 module,U32 id,unsigned char data)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32     
    char key[APS_PARA_KEY_NUM];
    nvs_handle my_handle;
    esp_err_t err;

    // Generate key
    AppKeyGen(module,id,key);
    
    // Open
    err = nvs_open(key, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {return err;}
    
    // Write
    err = nvs_set_u8(my_handle, key, data);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}
    
    // Commmit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {nvs_close(my_handle); return err;}

    // Close
    nvs_close(my_handle);
#else

#endif
    return 0;
}

void ItemMmtInit(void)
{
#if PLATFORM_DEFINE==PLATFORM_ESP32 
    // ESP NVS Init
    /*
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    */
#else
	
#endif
}




























