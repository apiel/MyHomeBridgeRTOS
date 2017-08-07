#ifndef __LWIPOPTS_BEFORE_NEXT_H__
#define __LWIPOPTS_BEFORE_NEXT_H__

#define LWIP_IGMP 1
#include <esp/hwrand.h>
#define LWIP_RAND hwrand

/* Use the defaults for everything else */
#include_next <lwipopts.h>

#endif