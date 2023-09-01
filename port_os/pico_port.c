/*
 * PicoTCP OS adapter
 *
 * Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
 */

#include "lib_debug/Debug.h"
#include "lib_mem/BitmapAllocator.h"
#include "lib_mem/AllocatorSafe.h"
#include "sel4/sel4.h"

#include "pico_port.h"

#include <stdlib.h>
#include <stdint.h>

// Profiling showed we need up to 1 MiB in this memory pool when picotcp
// has a loop score of 128
#define OS_NETWORK_STACK_MEMORY_POOL_SIZE  (1024*1024)
#define EL_SIZE     8
#define NUM_EL      (OS_NETWORK_STACK_MEMORY_POOL_SIZE / EL_SIZE)

// TODO: we have a hard dependency on the CAmkES names here. Once day this
//       should become a context structure that gets passed here, so the caller
//       must assigns the actual functions and it is the only entity to know
//       about CAmkES.
extern unsigned int Timer_getTimeMs(void);
extern int allocatorMutex_lock(void);
extern int allocatorMutex_unlock(void);
extern int nwstackMutex_lock(void);
extern int nwstackMutex_unlock(void);

//------------------------------------------------------------------------------
void pico_mutex_deinit(
    void* m)
{
    (void)m; // unused parameter
}


//------------------------------------------------------------------------------
void*
pico_mutex_init(void)
{
    // return a dummy context that is not zero or NULL
    return (void*)42;
}


//------------------------------------------------------------------------------
void
pico_mutex_lock(
    void* m)
{
    (void)m; // unused parameter

    // we don't check the return code here, because this is not supposed to
    // fail
    nwstackMutex_lock();
}


//------------------------------------------------------------------------------
void
pico_mutex_unlock(
    void* m)
{
    (void)m; // unused parameter

    // we don't check the return code here, because this is not supposed to
    // fail
    nwstackMutex_unlock();
}


//------------------------------------------------------------------------------
unsigned long
os_pico_time_ms(void)
{
    return Timer_getTimeMs();
}


//------------------------------------------------------------------------------
unsigned long
os_pico_time_s(void)
{
    // ToDo: check if we should better do rounding up or down here instead of
    //       always rounding down.
    return (os_pico_time_ms() / 1000);
}


//------------------------------------------------------------------------------
void
os_pico_idle(void)
{
    // TODO: do not call seL4 APIs directly, but define a wrapper OS_yield()
    seL4_Yield();
    return;
}


//------------------------------------------------------------------------------
static Allocator*
getAllocatorInstance(void)
{
    static Allocator* allocator = NULL;
    if (allocator != NULL)
    {
        return allocator;
    }


    static const Mutex const_allocMutex =
    {
        .lock   = allocatorMutex_lock,
        .unlock = allocatorMutex_unlock
    };
    // ToDo: AllocatorSafe_ctor() currently doesn't support passing a const
    //       object, until this gets fixed we use a helper variable.
    static Mutex* const allocMutex = (Mutex*)(&const_allocMutex);

    static AllocatorSafe    allocatorSafe;
    static BitmapAllocator  bmAllocator;

    static uint8_t bmap[BitmapAllocator_BITMAP_SIZE(NUM_EL)];
    static uint8_t bbmap[BitmapAllocator_BITMAP_SIZE(NUM_EL)];
    static uint8_t mem_pool[OS_NETWORK_STACK_MEMORY_POOL_SIZE];

    allocatorMutex_lock();

    do {
        bool ok;
        ok = Mutex_ctor((Mutex*) &allocMutex);
        if (!ok)
        {
            Debug_LOG_ERROR("Mutex_ctor() failed");
            break;
        }

        ok = BitmapAllocator_ctorStatic(
                &bmAllocator,
                mem_pool,
                bmap,
                bbmap,
                EL_SIZE,
                NUM_EL);
        if (!ok)
        {
            Debug_LOG_ERROR("BitmapAllocator_ctorStatic failed");
            break;
        }

        ok = AllocatorSafe_ctor(
                &allocatorSafe,
                BitmapAllocator_TO_ALLOCATOR(&bmAllocator),
                allocMutex);
        if (!ok)
        {
            Debug_LOG_ERROR("BitmapAllocator_ctorStatic failed");
            break;
        }

        allocator = AllocatorSafe_TO_ALLOCATOR(&allocatorSafe);

    } while(0);

    allocatorMutex_unlock();

    return allocator;
}


//------------------------------------------------------------------------------
void*
os_pico_zalloc(
    size_t n)
{
    Allocator* allocator = getAllocatorInstance();
    if (NULL == allocator)
    {
        Debug_LOG_ERROR("getAllocatorInstance() failed");
        return NULL;
    }

    void* mem = Allocator_alloc(allocator, n);
    if (NULL == mem)
    {
        Debug_LOG_ERROR("Allocator_alloc() failed for n=%zu", n);
        return NULL;
    }

    memset(mem, 0, n);
    return mem;
}


//------------------------------------------------------------------------------
void
os_pico_zfree(
    void* p)
{
    Allocator* allocator = getAllocatorInstance();
    if (NULL == allocator)
    {
        Debug_LOG_ERROR("getAllocatorInstance() failed");
        return;
    }

    Allocator_free(allocator, p);
}
