#ifndef __CONFIG_H__
#define __CONFIG_H__


#define CONFIG_SPECIFIED 1    /* 0:Use default config file,1:Use specified config file. */


#if CONFIG_SPECIFIED==1
#include "Config-specified.h" /* Each project own its specified config file. */
#else
#include "Config-default.h"   /* Default config file. */
#endif



#endif
