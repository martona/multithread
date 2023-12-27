/*
    see readme.md for some (thin) details
    (C) 2023 https://github.com/martona/multithread
    MIT License
*/

#include "multithread.h"
#include "submodules/getprocaddress/getprocaddress.c"
#define _MULTITHREAD_DEBUG 0

#if _MULTITHREAD_DEBUG
#define _WIN32_WINNT 0x0700
#include <windows.h>
#include <stdio.h>
#endif

//-------------------------------------------------------------------------------------
// data types required for determining core count

#pragma pack(push, 1)
typedef struct {
    ptr                     proc_mask;
    u32                     relationship;
    u32                     reserved1;
    u64                     reserved2[2];
} mt_proc_logical_info;
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////////////
// processor core count

#define mt_ERROR_INSUFFICIENT_BUFFER   122

int mt_get_cputhreads(mt_ctx* ctx) {
    i32 retlen          = 0;
    u32 logical_procs   = 0;
    u32 processor_cores = 0;

    mt_proc_logical_info* buffer = 0;
    while (!ctx->GetLogicalProcessorInformation(buffer, &retlen)) {
        if (ctx->GetLastError() == mt_ERROR_INSUFFICIENT_BUFFER) {
            if (buffer) 
                ctx->GlobalFree(buffer);
            if (!(buffer = (mt_proc_logical_info*)ctx->GlobalAlloc(mt_GPTR, retlen)))
                return 0;
        } else {
            return 0;
        }
    }
    mt_proc_logical_info* ptr = buffer;
    while (retlen >= sizeof(mt_proc_logical_info)) {
        if (ptr->relationship == 0) {
            processor_cores++;
            logical_procs += __builtin_popcountll((u64)ptr->proc_mask);
        }
        retlen -= sizeof(mt_proc_logical_info);
        ptr++;
    }

    #if _MULTITHREAD_DEBUG
        printf("GetLogicalProcessorInformation:\n");
        printf("cores: %d\n",  processor_cores);
        printf("threads: %d\n", logical_procs);
        printf("active: %d\n", GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
    #endif
    ctx->GlobalFree(buffer);
    return processor_cores;
}

