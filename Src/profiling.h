#ifndef _PROFILING_H
#define _PROFILING_H

#include "stm32f30x.h"
//#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdio.h>

#define MAX_EVENT_COUNT 20

void PROFILING_START(const char *profile_name);
void PROFILING_EVENT(const char *event);
void PROFILING_STOP(void);

#endif // _PROFILING_H
