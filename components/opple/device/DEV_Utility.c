#include <Includes.h>
#include <OPP_Debug.h>
#include <string.h>
#include <DEV_Utility.h>

////////////////////////////////////////////////////////////////

INT16U crc16(unsigned char *buf,unsigned short length)
{
  INT16U shift=0,data=0,val=0;
  int i=0;

  shift = CRC_SEED;


  for(i=0;i<length;i++) {
    if((i % 8) == 0)
      data = (*buf++)<<8;
    val = shift ^ data;
    shift = shift<<1;
    data = data <<1;
    if(val&0x8000)
      shift = shift ^ POLY16;
  }
  return shift;
} 
#if 0
/*
整形数据转字符串函数
        char *itoa(int value, char *string, int radix)
		radix=10 标示是10进制	非十进制，转换结果为0;  

	    例：d=-379;
		执行	itoa(d, buf, 10); 后
		
		buf="-379"							   			  
*/
char *itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    /* This implementation only works for decimal numbers. */
    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return string;

} /* NCL_Itoa */
#endif
#if 0
char *itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    /* This implementation only works for decimal numbers. */
    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return string;

} /* NCL_Itoa */
#endif
int isspace(int x)
{
	if(x==' '||x=='\t'||x=='\n'||x=='\f'||x=='\b'||x=='\r')
	  return 1;
	else  
	  return 0;
}

int isdigit(int x)
{
	if(x<='9'&&x>='0')         
	  return 1;
	else 
	  return 0;

}
int atoi(const char *nptr)
{
	int c;              /* current char */
	int total;         /* current total */
	int sign;           /* if '-', then negative, otherwise positive */

	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
	    ++nptr;

	c = (int)(unsigned char)*nptr++;
	sign = c;           /* save sign indication */
	if (c == '-' || c == '+')
	    c = (int)(unsigned char)*nptr++;    /* skip sign */

	total = 0;

	while (isdigit(c)) {
	    total = 10 * total + (c - '0');     /* accumulate digit */
	    c = (int)(unsigned char)*nptr++;    /* get next char */
	}

	if (sign == '-')
	    return -total;
	else
	    return total;   /* return result, negated if necessary */
}

/*Float ===> String*/
int ftoa(char *str, long double num, int n)        //n是转换的精度，即是字符串'.'后有几位小数
{
    int     sumI;
    long double   sumF;
    int     sign = 0;
    int     temp;
    int     count = 0;

    char *p;
    char *pp;

    if(str == NULL) return -1;
    p = str;

    /*Is less than 0*/
    if(num < 0)
    {
        sign = 1;
        num = 0 - num;
    }

    sumI = (int)num;    //sumI is the part of int
    sumF = num - sumI;  //sumF is the part of float

    /*Int ===> String*/
    do
    {
        temp = sumI % 10;
        *(str++) = temp + '0';
    }while((sumI = sumI /10) != 0);


    /*******End*******/
 

    if(sign == 1)
    {
        *(str++) = '-';
    }

    pp = str;
    
    pp--;
    while(p < pp)
    {
        *p = *p + *pp;
        *pp = *p - *pp;
        *p = *p -*pp;
        p++;
        pp--;
    }

    *(str++) = '.';     //point

    /*Float ===> String*/
    do
    {
        temp = (int)(sumF*10);
        *(str++) = temp + '0';

        if((++count) == n)
            break;
    
        sumF = sumF*10 - temp;

    }while(!(sumF > -0.000001 && sumF < 0.000001));

    *str = '\0';

    return 0;

}

float _atof(const char *str)
{
    float   sumF = 0;
    int     sumI = 0;
    int     sign = 0;

    if(str == NULL) return -1;
    
    /*Is less than 0 ?*/
    if(*str == '-')
    {
        sign = 1;
        str++;
    }

    /*The part of int*/
    while(*str != '.')
    {
        sumI = 10*sumI + (*str - '0');
        str++;
    }

    /*Let p point to the end*/
    while(*str != '\0')
    {
        str++;
    }

    str--;          //Your know!

    /*The part of float*/
    while(*str != '.')
    {
        sumF = 0.1*sumF + (*str - '0');
        str--;
    }

    sumF = 0.1*sumF;

    sumF += sumI;

    if(sign == 1)
    {
        sumF = 0 - sumF;
    }

    return sumF;

}

int string_count(char * str, char * substring)
{
    char *p;  
    int num=0;
	
	while (1)  
	{  
		p = strstr(str, substring);
		while (p != NULL)
		{
			num++;
			p = p + strlen(substring);  
			p = strstr(p, substring);	
		}
		return num;	
	}
}

int AINPUT(/*const int socket, */const char cmd, FUNC func, const unsigned short reqid, const char retry)
{
	int i = 0;

	printf("AINPUT %d\r\n", reqid);
	func(cmd, reqid);
	//retry--;
	
	for(i = 0; i < MAX; i++)
	{
		if(!g_astRetry[i].used)
		{
			g_astRetry[i].retry = (retry-1);//already run one time
			if(0 == g_astRetry[i].retry)
			{
				break;
			}
			//g_astRetry[i].socket = socket;			
			g_astRetry[i].cmd = cmd;
			g_astRetry[i].reqid = reqid;
			printf("AINPUT reqid %d %d\r\n", reqid, g_astRetry[i].reqid);
			g_astRetry[i].used = TRUE;
			g_astRetry[i].func = func;			
			break;
		}
	}

	return 0;
}

int AOUTPUT(unsigned short reqid)
{
	int i = 0;
	for(i = 0; i < MAX; i++)
	{
		if(g_astRetry[i].reqid == reqid)
		{
			//g_astRetry[i].socket = 0;		
			g_astRetry[i].cmd = 0;		
			g_astRetry[i].reqid = 0;
			g_astRetry[i].retry = 0;		
			g_astRetry[i].func = NULL;
			g_astRetry[i].used = FALSE;
			break;
		}
	}
	return 0;
}

int ACALL(u8 idx)
{
	if(g_astRetry[idx].used)
	{
		g_astRetry[idx].func(g_astRetry[idx].cmd, g_astRetry[idx].reqid);
		g_astRetry[idx].retry--;
		if(g_astRetry[idx].retry == 0)
		{
			//g_astRetry[idx].socket = 0;		
			g_astRetry[idx].cmd = 0;
			g_astRetry[idx].reqid = 0;
			g_astRetry[idx].retry = 0;
			g_astRetry[idx].used = FALSE;
		}
	}
	return 0;
}

int ARETRY(unsigned short reqid)
{
	int i = 0;
	for(i = 0; i < MAX; i++)
	{
		if(g_astRetry[i].reqid == reqid)
		{
			g_astRetry[i].retry--;
			if(g_astRetry[i].retry == 0)
			{
				g_astRetry[i].reqid = 0;
				g_astRetry[i].used = FALSE;
			}
			break;
		}
	}
	return 0;
}

/*
*0, is not zero
*1, is zero
*/
int IS_ZERO(const char * buffer, int len)
{
	int i = 0;

	for(i = 0; i < len; i++)
	{
		if(buffer[i] != 0)
			return 0;
	}

	return 1;
}

/*
*0, is not empty
*1, is empty
*/
int flashIsEmpty(const char * buffer, int len)
{
	int i = 0;
	
	for(i = 0; i < len; i++)
	{
		if(buffer[i] != 0x00)
			return 0;
	}

	return 1;
}
#define DEBUG
void RoadLamp_Dump(char * buf, int len)
{
#ifdef DEBUG
#define BUF_LEN    512
	int i = 0, bLen = 0;
	char buffer[BUF_LEN] ={0};
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "+++++++\r\n");
	for(i = 0; i < len; i++)
	{
		if(BUF_LEN - bLen < 5){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "%s", buffer);
			memset(buffer, 0, sizeof(buffer));
			bLen = 0;
		}
		bLen += sprintf(&buffer[bLen], "%02x ", buf[i]);
		
		if(bLen%16 == 0)
		{
			bLen += sprintf(&buffer[bLen], "%c%c", '\r','\n');
		}
	}
	
	if(BUF_LEN - bLen < 2){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "%s", buffer);
		memset(buffer, 0, sizeof(buffer));
		bLen = 0;
	}
	bLen += sprintf(&buffer[bLen], "%c%c", '\r','\n');
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "%s", buffer);
#endif	
}

