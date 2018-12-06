/*!
    \file  neul_bc95.c
    \brief NEUL BC95 IoT model driver 
*/

/*
    Copyright (C) 2016 GigaDevice

    2016-10-19, V1.0.0, demo for GD32F4xx
*/
#include "Includes.h"
#include <string.h>
#include <DRV_Bc95.h>
#include "DEV_Utility.h"
#include "SVS_Opple.h"
#include <string.h>

#if 0
static char neul_bc95_buf[NEUL_MAX_BUF_SIZE];
static char neul_bc95_wbuf[NEUL_MAX_BUF_SIZE];
static char neul_bc95_tmpbuf[NEUL_MAX_BUF_SIZE];

static long g_lSendPkt = 0;
//unsigned char imei[15];

#if 0
#define   HERE   printf("%s,%d\r\n", __FUNCTION__, __LINE__)
#else
#define   HERE
#endif
//static int uart_data_read(char *buf, int maxrlen, int mode);
//static int uart_data_write(const char *buf, int writelen, int mode);
int uart_data_read(char *buf, int maxrlen, int mode, int timeout);
int uart_data_write(char *buf, int writelen, int mode);


static neul_dev_operation_t neul_ops = {
    uart_data_read,
    uart_data_write
};
static remote_info udp_socket[NEUL_MAX_SOCKETS] = {
	{0, -1, {0}}
};
static neul_dev_info_t neul_dev = {
	{'\0'},
	{'\0'},
	{'\0'},
	{'\0'},
	{'\0'},
    neul_bc95_buf,
    neul_bc95_wbuf,
    0,
    udp_socket,
    &neul_ops,
    {0},
    0,
    0
};


/* ============================================================
func name   :   NeulBc28SetUpdParam
discription :   this func just called after the socket created
                set the remote ip address and port the socket sendto or receive fro
param       :   
             rip  @ input param, remote client/server ip address
             port @ input param, remote client/server port
             socket @ input param, local socket handle
return      :  0 mean ok, !0 means param err
Revision    : 
=============================================================== */
int NeulBc95SetUpdParam(const char *rip, const unsigned short port, const int socket)
{
    if (NULL == rip || 0 == port)
    {
        return -1;
    }
    if (strlen(rip) >= NEUL_IP_LEN)
    {
        return -2;
    }
    neul_dev.remotecount++;
    (neul_dev.addrinfo+socket)->port = port;
    (neul_dev.addrinfo+socket)->socket = socket;
    memcpy((neul_dev.addrinfo+socket)->ip, rip, strlen(rip));
    return 0;
}

/* ============================================================
func name   :   NeulBc95SetUpdParam
discription :   this func just called after the socket created
                set the remote ip address and port the socket sendto or receive fro
param       :   
             socket @ input param, local socket handle that need to reset
return      :  0 mean ok, !0 means param err
Revision    : 
=============================================================== */
static int NeulBc95ResetUpdParam(const int socket)
{
    if (socket < 0)
    {
        return -1;
    }
    neul_dev.remotecount--;
    (neul_dev.addrinfo+socket)->port = 0;
    (neul_dev.addrinfo+socket)->socket = -1;
    memset((neul_dev.addrinfo+socket)->ip, 0, NEUL_IP_LEN);
    return 0;
}

/* ============================================================
func name   :   NeulBc95SetCdpInfo
discription :   set cdp server ip address
param       :   
             cdp  @ input param, cdp server's ip address
return      :  0 mean ok, !0 means param err
Revision    : 
=============================================================== */
static int NeulBc95SetCdpInfo(const char *cdp)
{
    if(NULL == cdp)
    {
        return -1;
    }
    if (strlen(cdp) >= NEUL_IP_LEN)
    {
        return -2;
    }
    memcpy(neul_dev.cdpip, cdp, strlen(cdp));
    return 0;
}

/* ============================================================
func name   :  NeulBc95HwDetect
discription :  detect bc95 hardware 
param       :  None 
return      :  0 bc95 is connectted to mcu ok , !0 means not ok
Revision    : 
=============================================================== */
int NeulBc95HwDetect(void)
{
    int ret = 0;
    char *p = "AT+CGMM\r";
    char *cmd = "AT+CGMR\r";
    char *cmd0 = "AT+CGMI\r";
    char *str = NULL;
	
    ret = neul_dev.ops->dev_write(p, strlen(p), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (NULL == neul_dev.rbuf)
    {
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -2;
    }
    //NeulBc95Sleep(10);
    memset(neul_dev.rbuf, 0, NEUL_MANUFACT_LEN);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MANUFACT_LEN, 0, 200);
    if (ret <= 0)
    {
        //read bc95 manufacturer info failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -3;
    }

	printf("%s %s\n", p, neul_dev.rbuf);

#if 0
    str = strstr(neul_dev.rbuf,"Hi15RM1-HLB_V1.0");
    if (!str)
    {
        //can't find bc28 manufacturer info
        return -4;
    }

    str = strstr(neul_dev.rbuf,"BC28JA-02-STD");
    if (!str)
    {
        //can't find bc28 manufacturer info
        return -4;
    }
#endif

	//query version
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }

	printf("%s %s\n", cmd, neul_dev.rbuf);
	
	//query company
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc95 failed        
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }

	printf("%s %s\n", cmd0, neul_dev.rbuf);
	
    return 0;
}

/* ============================================================
func name   :  NeulBc95SetImei
discription :  set bc95 imei info 
param       :  
               imei @ input param, the imei data set to bc95
               len @ input param, lenght of the data
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetImei(const char *imei, int len)
{
    int ret = 0;
    char *p = "AT+NTSETID=1,";
    char *p2 = "AT+CGSN=1\r";
    char *str = NULL;
    //int cmdlen = 0;
    if (NULL == imei || len <= 0)
    {
        return -1;
    }
    if (NULL == neul_dev.wbuf)
    {
        return -2;
    }
    memset(neul_dev.wbuf, 0, 100);
    //cmdlen = strlen(p);
    //memcpy(neul_dev.wbuf, p, cmdlen);
    //memcpy(neul_dev.wbuf+cmdlen, imei, len);
    sprintf(neul_dev.wbuf, "%s%s%c", p, imei, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 16);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -3;
    }
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
        //read imei info
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
        if (ret < 0)
        {
            //write data to bc95 failed
            return -1;
        }
        memset(neul_dev.rbuf, 0, 64);
        ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 200);
        if (ret <= 0)
        {
            //read bc95 read set return value info failed
            return -3;
        }
        str = strstr(neul_dev.rbuf,"OK");
        if (NULL == str)
        {
            return -1;
        }
        str = strstr(neul_dev.rbuf,"+CGSN:");
        if (str && strlen(neul_dev.rbuf) >= 22)
        {
            return 0;
        }
        return -1;
    }
    return 0;
}

/* ============================================================
func name   :  int NeulBc95CheckImei
discription :  check if bc95 imei is setted 
param       :  None
return      :  1 imei already set, <=0 not set
Revision    : 
=============================================================== */
int NeulBc95CheckImei(void)
{
    int ret = 0;
    char *p2 = "AT+CGSN=1\r";
    char *str = NULL;
    int retry = 0;
    //int cmdlen = 0;

    if (NULL == neul_dev.wbuf)
    {
        return -2;
    }
    memset(neul_dev.wbuf, 0, 100);
    //cmdlen = strlen(p);
    //memcpy(neul_dev.wbuf, p, cmdlen);
    //memcpy(neul_dev.wbuf+cmdlen, imei, len);
		//read imei info
    while(retry < 3)
    {
        retry++;
		ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
		if (ret < 0)
		{
            //write data to bc95 failed
            return -1;
		}
		memset(neul_dev.rbuf, 0, 64);
		ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
		if (ret <= 0)
		{
            //read bc95 read set return value info failed
            return -3;
		}
		str = strstr(neul_dev.rbuf,"OK");
		if (NULL == str)
		{
            str = strstr(neul_dev.rbuf,"ERROR");
            if (NULL != str)
            {
                continue;
            }
		}
		str = strstr(neul_dev.rbuf,"+CGSN:");
		if (str && strlen(neul_dev.rbuf) >= 22)
		{
            return 1;
		}
    }
    return 0;
}


/* ============================================================
func name   :  NeulBc95SetBand
discription :  set bc95 work band 
param       :  
               band @ input param, bc95 work band BC95-CM set to 8, 
                      BC95-SL set to 5, BC95-VF set to 20
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetBand(int band)
{
    char *p = "AT+NBAND=";
    int ret = 0;
    char *str = NULL;
    memset(neul_dev.wbuf, 0, 16);
    if (band == 8 || band == 5 || band == 20)
    {
        sprintf(neul_dev.wbuf, "%s%d%c", p, band, '\r');
    }
    else
    {
        //error band for bc95
        printf("%s %s\n", __FUNCTION__, __LINE__);
        return -1;
    }
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s %s\n", __FUNCTION__, __LINE__);        
        return -1;
    }
    memset(neul_dev.rbuf, 0, 16);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s %s\n", __FUNCTION__, __LINE__);        
        return -3;
    }
	
	printf("%s %s\n", neul_dev.wbuf, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
        printf("%s %s\n", __FUNCTION__, __LINE__);    
        return -1;
    }
    return 0;
}




/* ============================================================
func name   :  NeulBc95SetFrequency
discription :  set bc95 work frequency 
param       :  0,信道自动选择
		      其他指定信道
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetFrequency(int channel)
{
    char *p = "AT+NEARFCN=";
    int ret = 0;
    char *str = NULL;

	printf("\r\n~~~~~~~~auto frequency start~~~~~~~~~\r\n");
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%d,%d%c", p, 0, channel, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 16);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -3;
    }
	
	printf("%s %s\n", neul_dev.wbuf, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
        return -1;
    }
	printf("\r\n~~~~~~~~auto frequency end~~~~~~~~~\r\n");
	
    return 0;
}


/* ============================================================
func name   :  NeulBc95GetBand
discription :  set bc95 work band 
param       :  
               band @ input param, bc95 work band BC95-CM set to 8, 
                      BC95-SL set to 5, BC95-VF set to 20
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95GetBand(void)
{
    char *p = "AT+NBAND?\r";
    int ret = 0;
    char *str = NULL;
	
    ret = neul_dev.ops->dev_write(p, strlen(p), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 16);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -3;
    }
	
	  printf("%s %s\n", p, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
        return -1;
    }
    return 0;
}

int NeulBc95QueryNccid(void)
{
    int ret = 0;
    char *str = NULL;	
    char *cmd = "AT+NCCID?\r";

    /* Get NCCID */
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed        
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }
	
	printf("%s %s\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"+NCCID:");
    if (!str)
    {
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

	strncpy(neul_dev.nccid, str, NEUL_NCCID_LEN-1);
	neul_dev.nccid[NEUL_NCCID_LEN] = '\0';

	return 0;
}
/* ============================================================
func name   :  NeulBc95ActiveNetwork
discription :  active bc95's network 
               this will take some time
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95ActiveNetwork(void)
{
    int ret = 0;
    char *str = NULL;
    char *cmd0 = "AT+CGSN=1\r";
    char *cmd1 = "AT+CFUN=1\r"; 
    char *cmd2 = "AT+CIMI\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"HUAWEI.COM\"\r";
    char *cmd3 = "AT+CGDCONT=1,\"IP\",\"psm0.eDRX0.ctnb\"\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"CTNB\"\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"PCCW\"\r";
    char *cmd4 = "AT+CGATT=1\r";
    char *cmd5 = "AT+CEREG=1\r";
    char *cmd6 = "AT+CSCON=1\r";

		
    /* Get IMEI */
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc95 failed        
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }

	
	printf("%s %s\n", cmd0, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"+CGSN:");
    if (!str)
    {
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
#if 1	
    if (strlen(str) < 25)
    {
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);    
        return -1;
    }
#endif	
	strncpy(neul_dev.imei, str+6, NEUL_IMEI_LEN-1);
	neul_dev.imei[NEUL_IMEI_LEN] = '\0';
//wangtao
#if 1    
    /* Config CFUN */
    ret = neul_dev.ops->dev_write(cmd1, strlen(cmd1), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }
    //NeulBc95Sleep(4000);//need wait 4 sconds or more, it's a little long
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 6, 0, 5000);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed        
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

	
	printf("%s %s\n", cmd1, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }    
 #endif
 
    NeulBc95Sleep(1000);
    /* Get IMSI */
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc95 failed        
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed       
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

	
	printf("%s %s\n", cmd2, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (strlen(neul_dev.rbuf) < 19)
    {
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);    
        return -1;
    }
	strncpy(neul_dev.imsi, neul_dev.rbuf, NEUL_IMEI_LEN-1);
	neul_dev.imsi[NEUL_IMEI_LEN] = '\0';
//set band error wangtao 

    /* set band to do... */
	/*电信*/
	NeulBc95GetBand();

    ret = NeulBc95SetBand(5);
    if (ret < 0)
    {
        return -1;
    }
    
#if 0
	/*联通*/
    ret = NeulBc95SetBand(8);
    if (ret < 0)
    {
        return -1;
    }
#endif
	NeulBc95GetBand();

    #if 1
    /* Config apn */
    ret = neul_dev.ops->dev_write(cmd3, strlen(cmd3), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -2;
    }
	
	printf("%s %s\n", cmd3, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
    #endif
    
    /* Active Network */
    ret = neul_dev.ops->dev_write(cmd4, strlen(cmd4), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	
	printf("%s %s\n", cmd4, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
    	printf("CGATT Not Attach\n");
        return -1;
    }
	
    /* CEREG */
    ret = neul_dev.ops->dev_write(cmd5, strlen(cmd5), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	
	printf("%s %s\n", cmd5, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
    	printf("CEREG Not Attach\n");
        return -1;
    }
    /* CSCON */
    ret = neul_dev.ops->dev_write(cmd6, strlen(cmd6), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	
	printf("%s %s\n", cmd6, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
    	printf("CSCON Not Attach\n");
        return -1;
    }

    return 0;
}


/* ============================================================
func name   :  NeulBc95DeactiveNetwork
discription :  active bc95's network 
               this will take some time
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95DeactiveNetwork(void)
{
    int ret = 0;
	int attach = 0;
    char *str = NULL;
    char *cmd0 = "AT+CGATT=0\r";
	
    /* Deactive Network */
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	
	printf("%s %s\n", cmd0, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
    	printf("CGATT Not Attach\n");
        return -1;
    }
	return 0;
}

/* ============================================================
func name   :  NeulBc95GetNetstat
discription :  get bc95's network status 
               
param       :  None
return      :  1 connected , 0 not connect
Revision    : 
=============================================================== */
int NeulBc95GetNetstat(void)
{
    char *cmd = "AT+CGATT?\r";
    char *str = NULL;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 500);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	printf("%s %s\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"CGATT:1");
    if (!str)
    {
        return -1;
    }
    
    return 0;
}

/* ============================================================
func name   :  NeulBc95GetCscon
discription :  get bc95's connect status 
               
param       :  None
return      :  0 connected , !0 not connect
Revision    : 
=============================================================== */
int NeulBc95GetCscon(void)
{
    char *cmd = "AT+CSCON?\r";
    char *cmd1 = "AT+CSCON=1\r";
    char *str = NULL;
	char ntype = 0;
	char mode = 0;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 500);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	printf("%s %s\n", cmd, neul_dev.rbuf);
	/*
	//+CSCON:1,0
    sscanf(neul_dev.rbuf, "\r\n+CSCON:%d,%d\r\n\r\nOK\r\n", &ntype, &mode);
	printf("ntype %d mode %d\r\n", ntype, mode);
	//connect state
	if(mode == 0)
	{
		return -1;
	}
	*/
	str = strstr(neul_dev.rbuf, "+CSCON:");
	if(!str)
	{
		sscanf(str, "+CSCON:%d,%d%*[\r\n]", &ret, &neul_dev.conStatus);
		printf("1111neul_dev.regStatus %d\r\n", neul_dev.conStatus);
	}
	
    return 0;
}

int NeulBc95GetCereg(void)
{
	int ret = 0;
    char *cmd = "AT+CEREG?\r";
    char *str = NULL;
	
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 500);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	printf("%s %s\n", cmd, neul_dev.rbuf);

	str = strstr(neul_dev.rbuf, "+CEREG:");
	if(!str)
	{
		sscanf(str, "+CEREG:%d,%d%*[\r\n]", &ret, &neul_dev.regStatus);
		printf("neul_dev.regStatus %d\r\n", neul_dev.regStatus);
	}
}
/* ============================================================
func name   :  NeulBc95QueryClock
discription :  query if bc95 get clock 
               
param       :  None
return      :  0 get clock , !0 no clock
Revision    : 
=============================================================== */
int NeulBc95QueryClock(char * time)
{
    char *cmd = "AT+CCLK?\r";
    char *str = NULL;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	//printf("%s %s\n", cmd, neul_dev.rbuf);

	strcpy(time, neul_dev.rbuf);
    
    return 0;
    
}

/* ============================================================
func name   :  NeulBc95QueryIp
discription :  query if bc95 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
int NeulBc95QueryIp(void)
{
    char *cmd = "AT+CGPADDR\r";
    char *str = NULL;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	printf("%s %s\n", cmd, neul_dev.rbuf);
	
    //str = strstr(neul_dev.rbuf,"CGPADDR:1");
    //str = strstr(neul_dev.rbuf,"+CGPADDR:0,");
    str = strstr(neul_dev.rbuf,"+CGPADDR:0,");
	if(str)
	{
		sscanf(str, "+CGPADDR:0,%s%[\r\n]", neul_dev.pdpAddr0);
		printf("\r\n~~~~~pdpaddr0 %s\r\n", neul_dev.pdpAddr0);
	}
    str = strstr(neul_dev.rbuf,"+CGPADDR:1,");
    if (str)
    {
    	sscanf(str, "+CGPADDR:1,%s%[\r\n]", neul_dev.pdpAddr1);
		printf("\r\n~~~~~pdpaddr1 %s\r\n", neul_dev.pdpAddr1);
    }
    if((0 == strlen(neul_dev.pdpAddr0)) && (0 == strlen(neul_dev.pdpAddr1)))
    {
		return -1;
	}
	
    return 0;
}


/* ============================================================
func name   :  NeulBc95Nconfig
discription :  query if bc95 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
int NeulBc95Nconfig(void)
{
    char *cmd = "AT+NCONFIG?\r";
    char *str = NULL;
    int ret = 0;

	//printf("%s start\r\n", cmd);
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
	
    memset(neul_dev.rbuf, 0, 256);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 256, 0, 500);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	printf("%s %s\n", cmd, neul_dev.rbuf);

	//printf("%s end\r\n", cmd);
	    
    return 0;
    
}

/* ============================================================
func name   :  NeulBc95QueryUestats
discription :  query if bc95 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
int NeulBc95QueryUestats(ST_NEUL_DEV * pstNeulDev)
{
    char *cmd = "AT+NUESTATS\r";
    char *str = NULL, *str1 = NULL;
    int ret = 0;
	char buffer[128] = {0};
	int len = 0;
	
	printf("\r\n~~~~~uestats start~~~~~~~\r\n");    
	if(NULL == pstNeulDev)
	{
		printf("pstNeulDev is NULL\r\n");
		return -1;
	}

	len = strlen(neul_dev.imei);
	if(0 < len){
		strncpy(pstNeulDev->imei, neul_dev.imei, len - 1);
		pstNeulDev->imei[len] = '\0';
	}
	len = strlen(neul_dev.imsi);
	if(0 < len) {
		strncpy(pstNeulDev->imsi, neul_dev.imsi, len - 1);
		pstNeulDev->imsi[len] = '\0';
	}
	len = strlen(neul_dev.nccid);
	if(0 < len){
		strncpy(pstNeulDev->nccid, neul_dev.nccid, len - 1);
		pstNeulDev->nccid[len] = '\0';
	}
	len = strlen(neul_dev.pdpAddr0);
	if(0 < len){
		strncpy(pstNeulDev->pdpAddr0, neul_dev.pdpAddr0, len - 1);
		pstNeulDev->pdpAddr0[len] = '\0';
	}
	len = strlen(neul_dev.pdpAddr1);
	if(0 < len) {
		strncpy(pstNeulDev->pdpAddr1, neul_dev.pdpAddr1, len - 1);
		pstNeulDev->pdpAddr1[len] = '\0';
	}

	pstNeulDev->regStatus = neul_dev.regStatus;
	pstNeulDev->conStatus = neul_dev.conStatus;
	//printf("%d %d\r\n", pstNeulDev->regStatus, neul_dev.regStatus);
	//printf("%d %d\r\n", pstNeulDev->conStatus, neul_dev.conStatus);

    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 256);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 256, 0, 500);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	/*
	AT+NUESTATS
	 
	Signal power:-988
	Total power:-760
	TX power:-32768
	TX time:0
	RX time:772
	Cell ID:227992403
	ECL:255
	SNR:55
	EARFCN:2509
	PCI:50
	RSRQ:-118
	OPERATOR MODE:0
	
	OK
	
	*/

	str = strstr(neul_dev.rbuf, "Signal power:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "Signal power:%d",  &pstNeulDev->rsrp);
			//printf("Signal power: %d\r\n", stNeulSignal.rsrp);
		}
	}
	str = strstr(neul_dev.rbuf, "Total power:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "Total power:%d",  &pstNeulDev->totalPower);
			//printf("Total power: %d\r\n", stNeulSignal.totalPower);
		}
	}
	
	str = strstr(neul_dev.rbuf, "TX power:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "TX power:%d",  &pstNeulDev->txPower);
			//printf("TX power: %d\r\n", stNeulSignal.txPower);
		}
	}
	str = strstr(neul_dev.rbuf, "TX time:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "TX time:%d",  &pstNeulDev->txTime);
			//printf("TX time: %d\r\n", stNeulSignal.txTime);
		}
	}
	str = strstr(neul_dev.rbuf, "RX time:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "RX time:%d",  &pstNeulDev->rxTime);
			//printf("RX time: %d\r\n", stNeulSignal.rxTime);
		}
	}

	str = strstr(neul_dev.rbuf, "Cell ID:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "Cell ID:%d",  &pstNeulDev->cellId);
			//printf("Cell ID: %d\r\n", stNeulSignal.rxTime);
		}
	}

	str = strstr(neul_dev.rbuf, "ECL:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "ECL:%d",  &pstNeulDev->signalEcl);
			//printf("ECL: %d\r\n", stNeulSignal.rxTime);
		}
	}
	str = strstr(neul_dev.rbuf, "SNR:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "SNR:%d",  &pstNeulDev->snr);
			//printf("SNR: %d\r\n", stNeulSignal.snr);
		}
	}
	str = strstr(neul_dev.rbuf, "EARFCN:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "EARFCN:%d",  &pstNeulDev->earfcn);
			//printf("EARFCN: %d\r\n", stNeulSignal.earfcn);
		}
	}
	str = strstr(neul_dev.rbuf, "PCI:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "PCI:%d",  &pstNeulDev->signalPci);
			//printf("PCI: %d\r\n", stNeulSignal.signalPci);
		}
	}
	str = strstr(neul_dev.rbuf, "RSRQ:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "RSRQ:%d",  &pstNeulDev->rsrq);
			//printf("RSRQ: %d\r\n", stNeulSignal.rsrq);
		}
	}
	str = strstr(neul_dev.rbuf, "OPERATOR MODE:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			strncpy(buffer, str, (str1-str));
			buffer[str1-str] = '\0';
			sscanf(buffer, "OPERATOR MODE:%d",  &pstNeulDev->opmode);
			//printf("OPERATOR MODE: %d\r\n", stNeulSignal.opmode);
		}
	}
	
	/*
	sscanf(neul_dev.rbuf, "\r\n\r\nSignal power:%d" \
						  "\r\nTotal power:%d" \
						  "\r\nTX power:%d" \
						  "\r\nTX time:%d" \
						  "\r\nRX time:%d" \
						  "\r\nCell ID:%d" \
						  "\r\nECL:%d" \
						  "\r\nSNR:%d" \
						  "\r\nEARFCN:%d" \
						  "\r\nPCI:%d" \
						  "\r\nRSRQ:%d" \
						  "\r\nOPERATOR MODE:%d" \
						  "\r\n\r\nOK\r\n",
						  &stNeulSignal.rsrp,
						  &stNeulSignal.totalPower,
						  &stNeulSignal.txPower,
						  &stNeulSignal.txTime,
						  &stNeulSignal.rxTime,
						  &stNeulSignal.cellId,
						  &stNeulSignal.signalEcl,
						  &stNeulSignal.snr,
						  &stNeulSignal.earfcn,
						  &stNeulSignal.signalPci,
						  &stNeulSignal.rsrq,
						  &stNeulSignal.opmode);
						  */
	/*printf("+++++++++++++++++++++++++++++++\r\n");
	printf("Signal power:%d" \
				  "\r\nTotal power:%d" \
				  "\r\nTX power:%d" \
				  "\r\nTX time:%d" \
				  "\r\nRX time:%d" \
				  "\r\nCell ID:%d" \
				  "\r\nECL:%d" \
				  "\r\nSNR:%d" \
				  "\r\nEARFCN:%d" \
				  "\r\nPCI:%d" \
				  "\r\nRSRQ:%d" \
				  "\r\nOPERATOR MODE:%d",
				  pstNeulDev->rsrp,
				  pstNeulDev->totalPower,
				  pstNeulDev->txPower,
				  pstNeulDev->txTime,
				  pstNeulDev->rxTime,
				  pstNeulDev->cellId,
				  pstNeulDev->signalEcl,
				  pstNeulDev->snr,
				  pstNeulDev->earfcn,
				  pstNeulDev->signalPci,
				  pstNeulDev->rsrq,
				  pstNeulDev->opmode);
	printf("+++++++++++++++++++++++++++++++\r\n");
*/
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	printf("%s %s\n", cmd, neul_dev.rbuf);
	printf("\r\n~~~~~uestats end~~~~~~~~~\r\n");    
	    
    return 0;
}

int NeulBc95QueryNetstats(U8 * regStatus, U8 * conStatus)
{
	*regStatus = neul_dev.regStatus;
	*conStatus = neul_dev.conStatus;
	printf("%d\r\n", neul_dev.regStatus);
	printf("%d\r\n", neul_dev.conStatus);

	return 0;
}
/* ============================================================
func name   :  NeulBc95QueryCsq
discription :  query if bc95 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
int NeulBc95QueryCsq(void)
{
    char *cmd = "AT+CSQ\r";
    char *str = NULL;
    int ret = 0;
    int rsrp = 0, ber = 0;
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	printf("%s %s\n", cmd, neul_dev.rbuf);
	//+CSQ:99,99
	/*
	sscanf(neul_dev.rbuf, "\r\n+CSQ:%d,%d\r\n\r\nOK\r\n", &rsrp, &ber);
	printf("rsrp %d ber %d\r\n", rsrp ,ber);
	//没信号
	if(rsrp == 99)
	{
		return -1;
	}
	*/
	
    return 0;
}

int NeulBc95Nping(const char *ipaddr)
{
    char *cmd = "AT+NPING=";
    char *str = NULL;
    int ret = 0;
    
    if (NULL == ipaddr || strlen(ipaddr) >= 16)
    {
        return -1;
    }
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%s%c", cmd, ipaddr, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 2000);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

	printf("%s %s", neul_dev.wbuf, neul_dev.rbuf);
	str = strstr(neul_dev.rbuf,"+NPING:");
	if (!str)
	{
		return -1;
	}

	return 0;
}
/* ============================================================
func name   :  NeulBc95Reboot
discription :  reboot bc95 
               
param       :  None
return      :  0 connected , !0 not connect
Revision    : 
=============================================================== */
int NeulBc95Reboot(void)
{
    char *cmd = "AT+NRB\r";
    char *str = NULL;
    int ret = 0;

	
	HERE;
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
	
	HERE;

    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 1000);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s,%d\r\n", __FUNCTION__, __LINE__); 
        return -1;
    }
	HERE;
	
	printf("%s %s\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"REBOOTING");
    if (!str)
    {
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);    
        return -1;
    }
    
    return 0;
}

int NeulBc95SetCfun(int flag)
{
    char *str = NULL;
    int ret = 0;
	char *cmd1 = "AT+CFUN=1\r";
	char *cmd2 = "AT+CFUN=0\r"; 
	char * p;

	if(flag)
		p = cmd1;
	else
		p = cmd2;
	/* Config CFUN */
	ret = neul_dev.ops->dev_write(p, strlen(p), 0);
	if (ret < 0)
	{
		//write data to bc95 failed
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);		
		return -1;
	}
	//NeulBc95Sleep(4000);//need wait 4 sconds or more, it's a little long
	memset(neul_dev.rbuf, 0, 32);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, 6, 0, 5000);
	if (ret <= 0)
	{
		//read bc95 read set return value info failed		 
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	printf("%s %s\n", p, neul_dev.rbuf);
	str = strstr(neul_dev.rbuf,"OK");
	if (!str)
	{
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
		return -1;
	}	 

	return 0;
}
/* ============================================================
func name   :  NeulBc95SetCdpserver
discription :  set bc95 cdp server 
               
param       :  ipaddr @ input param cdp server ip address
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetCdpserver(const char *ipaddr)
{
    char *cmd = "AT+NCDP=";
    char *cmd2 = "AT+NCDP?\r";
    char *str = NULL;
    int ret = 0;
    
    if (NULL == ipaddr || strlen(ipaddr) >= 16)
    {
        return -1;
    }
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%s%c", cmd, ipaddr, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

	printf("AT+NCDP=%s %s", ipaddr, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
    
    //query the setted ip back
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

	printf("%s %s", cmd2, neul_dev.rbuf);	
    str = strstr(neul_dev.rbuf,ipaddr);
    if (!str)
    {
        return -1;
    }
    
    return 0;
}

/* ============================================================
func name   :  NeulBc95HexToStr
discription :  convert char to hex string 
               
param       :  bufin @ input param , the data that need to convert
               len @ input param, data length
               bufout @ output param, hex string data 
return      :  0 convert ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc95HexToStr(const unsigned char *bufin, int len, char *bufout)
{
    int i = 0;
    #if 0
    int tmp = 0;
    #endif
    if (NULL == bufin || len <= 0 || NULL == bufout)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        #if 0
        tmp = bufin[i]>>4;
        bufout[i*2] = tmp > 0x09?tmp+0x37:tmp+0x30;
        tmp = bufin[i]&0x0F;
        bufout[i*2+1] = tmp > 0x09?tmp+0x37:tmp+0x30;
        #else
        sprintf(bufout+i*2, "%02X", bufin[i]);
        #endif
    }
    return 0; 
}

/* ============================================================
func name   :  NeulBc95HexToStr
discription :  convert hex string to hex data 
               
param       :  bufin @ input param , the data that need to convert
               len @ input param, data length
               bufout @ output param, hex data after convert 
return      :  0 send ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc95StrToHex(const char *bufin, int len, char *bufout)
{
    int i = 0;
    unsigned char tmp2 = 0x0;
    unsigned int tmp = 0;
    if (NULL == bufin || len <= 0 || NULL == bufout)
    {
        return -1;
    }
    for(i = 0; i < len; i = i+2)
    {
        #if 1
        tmp2 =  bufin[i];
        tmp2 =  tmp2 <= '9'?tmp2-0x30:tmp2-0x37;
        tmp =  bufin[i+1];
        tmp =  tmp <= '9'?tmp-0x30:tmp-0x37;
        bufout[i/2] =(tmp2<<4)|(tmp&0x0F);
        #else
        sscanf(bufin+i, "%02x", &tmp);
        bufout[i/2] = tmp;
        #endif
    }
    return 0; 
}

/* ============================================================
func name   :  NeulBc95SendCoapPaylaod
discription :  send coap message(hex data) to cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  0 send ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SendCoapPaylaod(const char *buf, int sendlen)
{
    char *cmd = "AT+NMGS=";
    char *cmd2 = "AT+NQMGS\r";
    int ret = 0;
    char *str = NULL;
    static int sndcnt = 0;
    int curcnt = 0;
    
    if (NULL == buf || sendlen >= 512)
    {
        return -1;
    }
		
    memset(neul_dev.wbuf, 0, 64);
    memset(neul_bc95_tmpbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = NeulBc95HexToStr((unsigned char *)buf, sendlen, neul_bc95_tmpbuf);
    sprintf(neul_dev.wbuf, "%s%d,%s%c", cmd, sendlen, neul_bc95_tmpbuf, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s write ret %d sendlen %d\n", cmd, ret, sendlen);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
	#if 0
	while(1)
	{
	    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
		#if 0
	    if (ret <= 0)
	    {
	        //read bc95 read set return value info failed
	        printf("%s %s error sendlen %d\n", cmd, neul_dev.rbuf, sendlen);
			RoadLamp_Dump(buf, sendlen);
	        return -1;
	    }
		#else
	    if (ret > 0)
	    {
	        printf("%s %s sendlen %d\n", cmd, neul_dev.rbuf, sendlen);
			break;
		}
		#endif
	}
	#else
	
	//ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 5000);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
	if (ret <= 0)
	{
		//read bc95 read set return value info failed
		printf("%s %s error sendlen %d\n", cmd, neul_dev.rbuf, sendlen);
		//RoadLamp_Dump(buf, sendlen);
		return -1;
	}
	#endif
	printf("%s %s", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
	if (!str)
	{
    	//printf("%s error", cmd);    
    	return -1;
	}

	
	g_lSendPkt++;

    /* query the message if send */
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("%s write ret %d", cmd2, ret);        
        return -1;
    }
    //ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 5000);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);	
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("%s %s error", cmd2, neul_dev.rbuf);        
        return -1;
    }

	printf("pkt %d %s %s \n", g_lSendPkt, cmd2, neul_dev.rbuf);
		
    str = strstr(neul_dev.rbuf,"SENT="); 
    if (!str)
    {
        return -1;
    }
		
    sscanf(str, "SENT=%d,%s", &curcnt, neul_dev.wbuf);
    if (curcnt == sndcnt)
    {
        return -1;
    }
    sndcnt = curcnt;
    return 0;
}

/* ============================================================
func name   :  NeulBc95ReadCoapMsg
discription :  recv coap message(hex data) from cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  >=0 is read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc95GetUnrmsgCount(void)
{
    char *cmd = "AT+NQMGR\r\n";
    int ret = 0;
    char *str = NULL;
    int msgcnt = 0;
    
    memset(neul_dev.rbuf, 0, 128);
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 128, 0, 100);
    if (ret < 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
	
    str = strstr(neul_dev.rbuf,"BUFFERED");
    if (!str)
    {
        return 0;
    }
    sscanf(neul_dev.rbuf, "\r\nBUFFERED=%d,%s", &msgcnt, neul_dev.wbuf);
    if (msgcnt < 0 )
    {
        return 0;
    }
	
	if (msgcnt > 0 )
	{
		printf("%s %s\n", cmd, neul_dev.rbuf);
	}
    return msgcnt;
}

/* ============================================================
func name   :  NeulBc95ReadCoapMsg
discription :  recv coap message(hex data) from cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  >=0 is read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc95ReadCoapMsg(char *outbuf, int maxrlen)
{
    char *cmd = "AT+NMGR\r\n";
    int ret = 0;
    char *str = NULL;
    int readlen = 0;

    if (NULL == outbuf || maxrlen == 0)
    {
        return -1;
    }

    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 540);
    if (ret < 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

	//printf("NMGR %s", neul_dev.rbuf);
	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str && strlen(neul_dev.rbuf) <=10 )
    {
        return 0;
    }
    sscanf(neul_dev.rbuf, "\r\n%d,%s\r\n\r\nOK\r\n",&readlen, neul_bc95_tmpbuf);
    if (readlen > 0)
    {
        NeulBc95StrToHex(neul_bc95_tmpbuf, readlen*2, outbuf);
        return readlen;
    }
    return 0;
}

/* ============================================================
func name   :  NeulBc95CreateUdpSocket
discription :  create udp socket 
               
param       :  localport @ input param , the port local socket used
return      :  >=0 socket handle , < 0 not ok
Revision    : 
=============================================================== */
int NeulBc95CreateUdpSocket(unsigned short localport)
{
    char *cmd = "AT+NSOCR=DGRAM,17,";
    int ret = 0;
    char *str = NULL;
    int socket = -1;
		int i = 0;
	
    if (0 == localport)
    {
        return -1;
    }
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%d,1\r", cmd, localport);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        printf("write %s error ret %d\r\n", neul_dev.wbuf, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        printf("read %s error ret %d\r\n", neul_dev.rbuf, ret);
        return -1;
    }
	/*
	printf("~~~~%s~~~~", neul_dev.rbuf);
    	str = strstr(neul_dev.rbuf,"OK");
    	if (!str)
    	{
        return -1;
    	}
	printf("#########\r\n");	
	for(i = 0; i<strlen(neul_dev.rbuf); i++)
	{
		printf("%02x ", neul_dev.rbuf[i]);
	}
	printf("!!!!!!!!!!\r\n");
	*/
    //sscanf(neul_dev.rbuf, "%d\r%s",&socket, neul_bc95_tmpbuf);
    sscanf(neul_dev.rbuf, "\nOK\r\n\r\n%d\r\n\r\nOK\r\n",&socket);
	printf("error11 socket %d\r\n", socket);
	
    if (socket >= 0)
    {
    	printf("socket %d\r\n", socket);
        return socket;
    }
    return -1;
}

/* ============================================================
func name   :  NeulBc95CloseUdpSocket
discription :  close udp socket 
               
param       :  socket @ input param , the socket handle that need close
return      :  0 close ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95CloseUdpSocket(int socket)
{
    char *cmd = "AT+NSOCL=";
    int ret = 0;
    char *str = NULL;
    
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%d\r", cmd, socket);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }

    NeulBc95ResetUpdParam(socket);
    
    return 0;
}

/* ============================================================
func name   :  NeulBc95SocketConfigRemoteInfo
discription :  set remote address info that socket will send data to 
               
param       :  socket @ input param , the socket handle that need close
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SocketConfigRemoteInfo(int socket, const char *remoteip, unsigned short port)
{
    int ret = 0;
    if (socket < 0 || NULL == remoteip || port == 0)
    {
        return -1;
    }
    ret = NeulBc95SetUpdParam(remoteip, port, socket);
    return ret;
}

/* ============================================================
func name   :  NeulBc95UdpSend
discription :  send data to socket 
               
param       :  socket @ input param , the data will send to this socket
               buf @ input param, the data buf
               sendlen @ input param, the send data length
return      :  0 send data ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95UdpSend(int socket, const char *buf, int sendlen)
{
    //AT+NSOST=0,192.53.100.53,5683,25,400241C7B17401724D0265703D323031363038323331363438
    char *cmd = "AT+NSOST=";
    int ret = 0;
    char *str = NULL;
    
    sscanf(neul_dev.rbuf, "%d\r%s",&socket, neul_bc95_tmpbuf);
    if (socket < 0 || NULL == buf || 0 == sendlen)
    {
        return -1;
    }
    
    memset(neul_dev.wbuf, 0, NEUL_MAX_BUF_SIZE);
    memset(neul_bc95_tmpbuf, 0, NEUL_MAX_BUF_SIZE);
    NeulBc95HexToStr((unsigned char *)buf, sendlen, neul_bc95_tmpbuf);
    sprintf(neul_dev.wbuf, "%s%d,%s,%d,%d,%s\r", cmd, socket,
            (neul_dev.addrinfo+socket)->ip,
            (neul_dev.addrinfo+socket)->port,
            sendlen,
            neul_bc95_tmpbuf);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
    return 0;
}

/* ============================================================
func name   :  NeulBc95UdpRead
discription :  read data from socket 
               
param       :  socket @ input param , the data will read from this socket
               buf @ out param, store the data read
               maxrlen @ input param, the max read data length
return      :  >0 read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc95UdpRead(int socket,char *buf, int maxrlen, int mode)
{
    //AT+NSORF=0,4 
    char *cmd = "AT+NSORF=";
    int ret = 0;
    char *str = NULL;
    int rlen = 0;
    int rskt = -1;
    int port = 0;
    int readleft = 0;
	  int i = 0;
	  char buf1[300] = {0};
	  int dot1, dot2, dot3, dot4;
	  int len = 0;
	  int sock, len1;
	
	  //printf("%s %d\r\n", __FUNCTION__, __LINE__);	

    if (socket < 0 || NULL == buf || maxrlen <= 0)
    {
		    printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    
    sprintf(neul_dev.wbuf, "%s%d,%d\r", cmd, socket, maxrlen);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
		printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    memset(neul_dev.rbuf, 0, 100);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 100, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
		printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
	printf("%s %s\r\n", neul_dev.wbuf, neul_dev.rbuf);
#if 0	
	for(;;)
	{
		if(neul_dev.rbuf[i] == '\0')
			break;
		printf("0x%x ", neul_dev.rbuf[i]);
		//printf("%c ", neul_dev.rbuf[i]);
		i++;
	}
	printf("\r\n");
#endif	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }

	str = strstr(neul_dev.rbuf, "\r\n+NSONMI:");
	/*if(!str)
	{
		return -1;
	}
	*/
	if(str)
	{
	#if 1
		len = str - neul_dev.rbuf;
		if(len == 0)
		{
			sscanf(neul_dev.rbuf, "\r\n+NSONMI:%d,%d\r\n\r\n%d,%d.%d.%d.%d,%d,%d,%s,%d\r\n%s\r\n\r\n", 
									&sock,
									&len1,
									&rskt,
									&dot1,
									&dot2,
									&dot3,
									&dot4,
									&port,
									&rlen,
									neul_bc95_tmpbuf,
									&readleft,
									neul_dev.wbuf);
			
			printf("+++buf1 %s\r\n", neul_bc95_tmpbuf);			
			
			if (rlen > 0)
			{
				printf("+++>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);			
				NeulBc95StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
			}
			
			return rlen;
		}
		else
#endif
		{
			strncpy(buf1, neul_dev.rbuf, str - neul_dev.rbuf);
			buf1[str - neul_dev.rbuf] = '\0';
		}
	}
	else
	{
		strncpy(buf1, neul_dev.rbuf, strlen(neul_dev.rbuf));
		buf1[strlen(neul_dev.rbuf)] = '\0';
	}
		
	printf("!!!buf1 %s\r\n", buf1);
	
    sscanf(/*neul_dev.rbuf*/buf1, "\r\n%d,%d.%d.%d.%d,%d,%d,%s,%d\r\n%s\r\n\r\n", 
							&rskt,
                            &dot1,
                            &dot2,
                            &dot3,
                            &dot4,
                            &port,
                            &rlen,
                            neul_bc95_tmpbuf,
                            &readleft,
                            neul_dev.wbuf);
	
    if (rlen > 0)
    {
		printf("==>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);    
        NeulBc95StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
    }
    
    return rlen;
}


/* ============================================================
func name   :  NeulBc95UdpNewRead
discription :  read data from socket 
               
param       :  socket @ input param , the data will read from this socket
               buf @ out param, store the data read
               maxrlen @ input param, the max read data length
return      :  >0 read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc95UdpNewRead(int socket,char *buf, int maxrlen, int mode, int * rleft)
{
    //AT+NSORF=0,4
    char *str = NULL;    
    char *cmd = "AT+NSORF=";
    int ret = 0;
    int rlen = 0;
    int rskt = -1;
    int port = 0;
    static int readleft = 0;
	int i = 0;
	int dot1, dot2, dot3, dot4;
	int len = 0;
	int sock, len1;
	int num;
	char *buf2 = "\r\nOK\r\n";
	
	//printf("%s %d\r\n", __FUNCTION__, __LINE__);
    if (socket < 0 || NULL == buf || maxrlen <= 0)
    {
		printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    
    sprintf(neul_dev.wbuf, "%s%d,%d\r", cmd, socket, maxrlen);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
		printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    memset(neul_dev.rbuf, 0, maxrlen);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, maxrlen, 0, 200);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
		printf("%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
	
	//printf("\r\n***********\r\n%s\r\n%s\r\n**************\r\n", neul_dev.wbuf, neul_dev.rbuf);


    str = strstr(neul_dev.rbuf,"\r\n+CSCON:");
    if (str)
    {
        return -1;
    }

	if(strcmp(neul_dev.rbuf, buf2) == 0)
	{
		*rleft = 0;
		return 0;
	}
	str = strstr(neul_dev.rbuf, "\r\n+NSONMI:");
	/*if(!str)
	{
		return -1;
	}
	*/
	if(str)
	{
		len = str - neul_dev.rbuf;
		if(len == 0)
		{
			sscanf(neul_dev.rbuf, "\r\n+NSONMI:%d,%d\r\n", 
									&sock,
									&readleft);
			
			//printf("1111 neul_bc95_tmpbuf %s\r\n", neul_bc95_tmpbuf);			
			/*
			if (rlen > 0)
			{
				printf("+++>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);			
				NeulBc95StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
			}
			*/
			
			//printf("111 NSONMI sock %d len %d\r\n", sock, readleft);

			*rleft = readleft;			
			return 0;
		}
		else
		{
		#if 0
			strncpy(buf1, neul_dev.rbuf, str - neul_dev.rbuf);
			buf1[str - neul_dev.rbuf] = '\0';
		#else
			sscanf(neul_dev.rbuf, "\r\nOK\r\n+NSONMI:%d,%d\r\n\r\n", 
								&sock,
								&readleft);
			//printf("222 NSONMI sock %d len %d\r\n", sock, readleft);

			*rleft = readleft; 
			return 0;
		#endif
		}
	}
/*
	else
	{
		strncpy(buf1, neul_dev.rbuf, strlen(neul_dev.rbuf));
		buf1[strlen(neul_dev.rbuf)] = '\0';
	}
*/		
	//printf("!!!neul_dev.rbuf %s\r\n", neul_dev.rbuf);

	num = string_count(neul_dev.rbuf, ",");

	//printf("\r\nnum %d\r\n", num);

	memset(neul_bc95_tmpbuf, 0, sizeof(neul_bc95_tmpbuf));
	
	if(num == 4)
	{
	    sscanf(neul_dev.rbuf, "\r\n%d,%d.%d.%d.%d,%d,%d,%s", 
								&rskt,
	                            &dot1,
	                            &dot2,
	                            &dot3,
	                            &dot4,
	                            &port,
	                            &rlen,
	                            neul_bc95_tmpbuf);
		if(strlen(neul_bc95_tmpbuf) < rlen)
		{
			*rleft = readleft - strlen(neul_bc95_tmpbuf);
		}
		else
		{
			*rleft = readleft - rlen;
		}
		
		//printf("4 neul_bc95_tmpbuf rlen %d rleft %d %s\r\n", strlen(neul_bc95_tmpbuf), *rleft, neul_bc95_tmpbuf);
	}
	if(num == 5)
	{
	    sscanf(neul_dev.rbuf, "\r\n%d,%d.%d.%d.%d,%d,%d,%[^,],%d\r\nOK\r\n", 
								&rskt,
	                            &dot1,
	                            &dot2,
	                            &dot3,
	                            &dot4,
	                            &port,
	                            &rlen,
	                            neul_bc95_tmpbuf,
	                            &readleft);
		*rleft = readleft;
		//printf("5 neul_bc95_tmpbuf rlen %d rleft %d %s\r\n", rlen, *rleft, neul_bc95_tmpbuf);
	}
	if(num == 0)
	{
		rlen = strlen(neul_dev.rbuf);
		strncpy(neul_bc95_tmpbuf, neul_dev.rbuf, strlen(neul_dev.rbuf));
		neul_bc95_tmpbuf[strlen(neul_dev.rbuf)] = '\0';

		*rleft = readleft - rlen;		
		//printf("0 neul_bc95_tmpbuf rlen %d rfetft %d %s\r\n", rlen, *rleft, neul_bc95_tmpbuf);
	}
	
    if (rlen > 0)
    {
		//printf("==>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);    
        NeulBc95StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
    }
    
    return rlen;
}

/* ============================================================
func name   :  NeulBc95SetAutoConnect
discription :  set bc95 module auto connect network
               
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetAutoConnect(int flag)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT+NCONFIG=AUTOCONNECT,TRUE\r";
    char *p2 = "AT+NCONFIG=AUTOCONNECT,FALSE\r";

    if (flag)
    {
        //set auto connect
        ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
    }
    else
    {
        //not auto connect
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

    if (flag)
	{
		printf("%s %s\n", p1, neul_dev.rbuf);
    }
	else
	{
		printf("%s %s\n", p2, neul_dev.rbuf);
	}
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
    return 0;
}

/* ============================================================
func name   :  NeulBc95SetScramble
discription :  set bc95 module scramble
               
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetScramble(int flag)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT+NCONFIG=CR_0354_0338_SCRAMBLING,TRUE\r";
    char *p2 = "AT+NCONFIG=CR_0354_0338_SCRAMBLING,FALSE\r";
    char *p3 = "AT+NCONFIG=CR_0859_SI_AVOID,TRUE\r";
    char *p4 = "AT+NCONFIG=CR_0859_SI_AVOID,FALSE\r";

	//CR_0354_0338_SCRAMBLING
    if (flag)
    {
        //set CR_0354_0338_SCRAMBLING trte
        ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
		
    }
    else
    {
        //CR_0354_0338_SCRAMBLING false
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

    if (flag)
	{
		printf("%s %s\n", p1, neul_dev.rbuf);
    }
	else
	{
		printf("%s %s\n", p2, neul_dev.rbuf);
	}
	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
	
	//SI_AVOID
    if (flag)
    {
        //set SI_AVOID true
        ret = neul_dev.ops->dev_write(p3, strlen(p3), 0);
		
    }
    else
    {
        //set SI_AVOID false
        ret = neul_dev.ops->dev_write(p4, strlen(p4), 0);
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
        return -1;
    }

    if (flag)
	{
		printf("%s %s\n", p3, neul_dev.rbuf);
    }
	else
	{
		printf("%s %s\n", p4, neul_dev.rbuf);
	}
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
	
    return 0;
}

/* ============================================================
func name   :  NeulBc95SetScramble
discription :  set bc95 module scramble
               
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95SetReselection(int flag)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT+NCONFIG=COMBINE_ATTACH,TRUE\r";
    char *p2 = "AT+NCONFIG=COMBINE_ATTACH,FALSE\r";
    char *p3 = "AT+NCONFIG=CELL_RESELECTION,TRUE\r";
    char *p4 = "AT+NCONFIG=CELL_RESELECTION,FALSE\r";
	char *p = NULL;
	
	//CR_0354_0338_SCRAMBLING
    if (flag)
    {
        //set COMBINE_ATTACH true
        ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
		p = p1;
    }
    else
    {
        //set COMBINE_ATTACH false
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
		p = p2;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
		printf("%s write error\r\n", p);        
        return -1;
    }

    if (flag)
	{
		printf("%s %s\n", p1, neul_dev.rbuf);
    }
	else
	{
		printf("%s %s\n", p2, neul_dev.rbuf);
	}
	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
	
	//SI_AVOID
    if (flag)
    {
        //set CELL_RESELECTION true
        ret = neul_dev.ops->dev_write(p3, strlen(p3), 0);
		p = p3;
    }
    else
    {
        //set CELL_RESELECTION false
        ret = neul_dev.ops->dev_write(p4, strlen(p4), 0);
		p = p4;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
		printf("%s write error\r\n", p);        
        return -1;
    }

    if (flag)
	{
		printf("%s %s\n", p3, neul_dev.rbuf);
    }
	else
	{
		printf("%s %s\n", p4, neul_dev.rbuf);
	}
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
	
    return 0;
}


/* ============================================================
func name   :  NeulBc95HwInit
discription :  init neul bc95 device 
               
param       :  None
return      :  0 init ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc95HwInit(void)
{
    int ret = 0;

	HERE;
    NeulBc95Reboot();
	HERE;
	NeulBc95Sleep(100);    
	HERE;
	
	neul_bc28_query_natspeed();
	neul_bc28_set_natspeed();
	neul_bc28_query_natspeed();

    ret = NeulBc95HwDetect();
    if (ret < 0)
    {
        printf("%s,%d\r\n", __FUNCTION__, __LINE__);    
        return ret;
    }
	
	//NeulBc95DeactiveNetwork();
 	//NeulBc95PrepareTransmit();
    //not auto connect
    NeulBc95SetAutoConnect(1);
    NeulBc95SetScramble(1);
    //NeulBc95SetScramble(0);
    //版本大于657sp2
    NeulBc95SetReselection(1);
    //NeulBc95SetReselection(0);
	NeulBc95Nconfig();
    NeulBc95Sleep(100);

  	//NeulBc95PrepareTransmit();
    //NeulBc95Sleep(200);
	//wangtao error here
	NeulBc95SetFrequency(2509);
    ret = NeulBc95ActiveNetwork();
    
    return ret;
}
int NeulBc95PrepareTransmit(void)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT\r\n";
    
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 500);

	
	//printf("AT %s", neul_dev.rbuf);

    uart_data_flush();
    return ret;
}
int NeulBc95Sleep(int ticks)
{
    LOS_TaskDelay(ticks);
    return 0;
}

//AT+NTSETID=1,460012345678966

//\r\nNeul \r\nOK\r\n
//typedef int (*neul_read)(char *buf, int maxrlen, int mode);
//typedef int (*neul_write)(const char *buf, int writelen, int mode);


//static int uart_data_read(char *buf, int maxrlen, int mode)
int uart_data_read(char *buf, int maxrlen, int mode, int timeout)
{
  int retlen = 0;
	char *str = NULL;
	int temp = 0, temp0 = 0;
	char buf1[32] = {0};
	
    if (NULL == buf || maxrlen <= 0 || timeout < 0)
    {
        return 0;
    }
    if(mode == 0)
    {
        //block mode
        retlen = UartRead(LOS_STM32F410_USART2, buf, maxrlen, timeout);
    }
    else
    {
        //none block mode
        retlen = UartRead(LOS_STM32F410_USART2, buf, maxrlen, 0);
    }
	if(retlen > 0)
	{
		str = strstr(buf, "+CSCON:");
		if(str)
		{
			sscanf(str, "%s*[\r\n]", buf1);

			if(strchr(str, ','))
				sscanf(str, "+CSCON:%d,%u*[\r\n]", &temp, &neul_dev.conStatus);
			else
				sscanf(str, "+CSCON:%u*[\r\n]", &neul_dev.conStatus);		
			printf("\r\n+++++++neul_dev.conStatus %u+++++++\r\n", neul_dev.conStatus);
		}
		
		str = strstr(buf, "+CEREG:");
		if(str)	
		{
			sscanf(str, "%s*[\r\n]", buf1);

			if(strchr(str, ','))
				sscanf(str, "+CEREG:%u,%d*[\r\n]", &temp, &neul_dev.regStatus);
			else
				sscanf(str, "+CEREG:%u*[\r\n]", &neul_dev.regStatus);		
		
			printf("\r\n+++++++neul_dev.regStatus %u+++++++\r\n", neul_dev.regStatus);
		}
	}
    return retlen;
}

int neul_data_process(char *outbuf, int maxrlen)
{
	int rlen = 0;

	if(NULL == outbuf)
	{
		return -1;
	}
	
	rlen = NeulBc95ReadData((char *)outbuf, maxrlen);
	if(rlen > 0)
	{
		//int AppMsgWrite(app_msg_chl chl,unsigned char* data,unsigned char len);
	}

	return 0;
}

int neul_data_send(const char *buf, int sendlen)
{
	//int AppMsgRead(app_msg_chl* chl,unsigned char* data,unsigned char* len);
	NeulBc95SendCoapPaylaod(buf, sendlen);
}
//static int uart_data_write(const char *buf, int writelen, int mode)
int uart_data_write(char *buf, int writelen, int mode)
{
    if (NULL == buf || writelen <= 0)
    {
        return 0;
    }
    UartWrite(LOS_STM32F410_USART2, buf, writelen, 500);
    return 0;
}

int uart_data_flush(void)
{
    memset(neul_bc95_buf, 0, NEUL_MAX_BUF_SIZE);
    return 0;
}
#endif
