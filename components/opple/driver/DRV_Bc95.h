#ifndef _NEUL_BC95_H
#define _NEUL_BC95_H

#define NEUL_MAX_BUF_SIZE 1064
#define NEUL_MAX_SOCKETS 1 // the max udp socket links
#define NEUL_IP_LEN 16
#define NEUL_IPV6_LEN 46
#define NEUL_MANUFACT_LEN 20
#define NEUL_IMEI_LEN    16
#define NEUL_IMSI_LEN    16
#define NEUL_NCCID_LEN   21

typedef int (*neul_read)(char *buf, int maxrlen, int mode, int timeout);
typedef int (*neul_write)(char *buf, int writelen, int mode);

typedef struct _neul_dev_operation_t 
{
    neul_read dev_read;
    neul_write dev_write;
} neul_dev_operation_t;

typedef struct _remote_info_t
{
    unsigned short port;
    int socket;
    char ip[16];
}remote_info;

typedef struct _neul_dev_info_t 
{
	U8 imei[NEUL_IMEI_LEN];
	U8 imsi[NEUL_IMSI_LEN];
	U8 nccid[NEUL_NCCID_LEN];
	U8 pdpAddr0[NEUL_IPV6_LEN];
	U8 pdpAddr1[NEUL_IPV6_LEN];
    char *rbuf;
    char *wbuf;
    int remotecount;
    remote_info *addrinfo;
    neul_dev_operation_t *ops;
    char cdpip[16];
	U8 regStatus; /*
					0 Not registered, UE is not currently searching an operator to register to
					1 Registered, home network
					2 Not registered, but UE is currently trying to attach or searching an operator to register to
					3 Registration denied
					4 Unknown (e.g. out of E-UTRAN coverage)
					5 Registered, roaming
				   */
	U8 conStatus; /*0 Idle, 1 Connected*/
} neul_dev_info_t;

typedef struct
{
	U8 imei[NEUL_IMEI_LEN];
	U8 imsi[NEUL_IMSI_LEN];
	U8 nccid[NEUL_NCCID_LEN];
	U8 pdpAddr0[NEUL_IPV6_LEN];
	U8 pdpAddr1[NEUL_IPV6_LEN];
	S32 rsrp;
	S32 totalPower;
	S32 txPower;
	U32 txTime;
	U32 rxTime;
	U32 cellId;
	U32 signalEcl;
	S32 snr;
	U32 earfcn;
	U32 signalPci;
	S32 rsrq;
	U8  opmode;
/*0 Not registered, UE is not currently searching an operator to register to
					1 Registered, home network
					2 Not registered, but UE is currently trying to attach or searching an operator to register to
					3 Registration denied
					4 Unknown (e.g. out of E-UTRAN coverage)
					5 Registered, roaming*/	
	U8 regStatus; 
	U8 conStatus; /*0 Idle, 1 Connected*/	
}ST_NEUL_DEV;

//extern unsigned char imei[15];

int NeulBc95SetUpdParam(const char *rip, const unsigned short port, const int socket);
int NeulBc95HwDetect(void);
int NeulBc95SetImei(const char *imei, int len);
int NeulBc95SetBand(int band);
int NeulBc95GetBand(void);
int NeulBc95SetFrequency(int channel);
int NeulBc95QueryNccid(void);
int NeulBc95ActiveNetwork(void);
int NeulBc95DeactiveNetwork(void);
int NeulBc95GetNetstat(void);
int NeulBc95GetCscon(void);
int NeulBc95GetCereg(void);
int NeulBc95QueryClock(char * time);
int NeulBc95QueryIp(void);
int NeulBc95Nconfig(void);
int NeulBc95QueryUestats(ST_NEUL_DEV * pstNeulSignal);
int NeulBc95QueryNetstats(U8 * regStatus, U8 * conStatus);
int NeulBc95QueryCsq(void);
int NeulBc95Nping(const char *ipaddr);
int NeulBc95Reboot(void);
int NeulBc95SetCfun(int flag);
int NeulBc95SetCdpserver(const char *ipaddr);
int neul_bc28_huawei_data_encryption(void);
int neul_bc28_huawei_iot_set_regmod(int flag);
int neul_bc28_huawei_iot_query_reg(void);
int neul_bc28_iot_register_data(void);
int neul_bc28_read_data(char *outbuf, int maxrlen);
int neul_bc28_huawei_iot_registration(const int flag);
int neul_bc28_query_natspeed(void);
int neul_bc28_set_natspeed(void);
int neul_bc28_send_coap_paylaod(const char *buf, int sendlen);
int NeulBc95SendCoapPaylaod(const char *buf, int sendlen);
int NeulBc95GetUnrmsgCount(void);
int NeulBc95ReadCoapMsg(char *outbuf, int maxrlen);
int neul_bc28_read_coap_msg(char *outbuf, int maxrlen);
int NeulBc95CreateUdpSocket(unsigned short localport);
int NeulBc95CloseUdpSocket(int socket);
int NeulBc95SocketConfigRemoteInfo(int socket, const char *remoteip, unsigned short port);
int NeulBc95UdpSend(int socket, const char *buf, int sendlen);
int NeulBc95UdpRead(int socket,char *buf, int maxrlen, int mode);
int NeulBc95UdpNewRead(int socket,char *buf, int maxrlen, int mode, int * rleft);
int NeulBc95HwInit(void);
int neul_bc95_create_data_task(void);
int NeulBc95SetAutoConnect(int flag);
int NeulBc95SetScramble(int flag);
int NeulBc95SetReselection(int flag);
int NeulBc95Sleep(int ticks);
int NeulBc95PrepareTransmit(void);
int uart_data_flush(void);
#endif /* HELLO_GIGADEVICE_H */
