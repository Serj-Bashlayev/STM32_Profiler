#ifndef _PROFILING_H
#define _PROFILING_H

#include <stdint.h>
#include <stdio.h>
#include "stm32f30x.h"

#define MAX_PROFILING_EVENT_COUNT 10
#define FCPU 72000000

void PROFILING_START(const char *profile_name); 
void PROFILING_EVENT(const char *event);
void PROFILING_STOP(void);

#endif // _PROFILING_H
