/*
 * PicoTCP OS port
 *
 * PicoTCP will look for this header file if PICO_PORT_CUSTOM is set
 *
 * Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
 */

#pragma once

#include "lib_debug/Debug.h"

#define dbg(...) Debug_LOG_TRACE(__VA_ARGS__)

unsigned long os_pico_time_s(void);
#define PICO_TIME() os_pico_time_s()

unsigned long os_pico_time_ms();
#define PICO_TIME_MS() os_pico_time_ms()

void os_pico_idle(void);
#define PICO_IDLE() os_pico_idle()

void* os_pico_zalloc(size_t n);
#define pico_zalloc(x) os_pico_zalloc(x)

void os_pico_zfree(void *p);
#define pico_free(x) os_pico_zfree(x)
