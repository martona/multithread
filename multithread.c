/*
    see readme.md for some (thin) details
    (C) 2023 https://github.com/martona/multithread
    MIT License
*/

#define _MULTITHREAD_DEBUG 0
#include "multithread.h"
#include "submodules/getprocaddress/getprocaddress.c"
#include "multithread_good.c"
#include "multithread_mspool.c"

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
    u32 efficient_cores = 0;

    typedef enum {
        mt_cpu_set_information_type
    } _mt_cpu_set_information_type;

    #pragma pack(push, 1)
    typedef struct {
        u32                          size;
        _mt_cpu_set_information_type type;
        union {
            struct {
                u32                 id;
                u16                 group;
                u8                  logical_proc_idx;
                u8                  core_idx;
                u8                  last_level_cache_idx;
                u8                  numa_node_idx;
                u8                  efficiency_class;
                union {
                    u8              all_flags;
                    struct {
                        u8          parked : 1;
                        u8          allocated : 1;
                        u8          allocated_to_target_process : 1;
                        u8          real_time : 1;
                        u8          reserved_flags : 4;
                    };
                }; // anonymous union
                union {
                    u32             reserved;
                    u8              scheduling_class;
                };
                u64                 allocation_tag;
            } cpu_set;
        }; // anonymous union
    } mt_cpu_set_information;
    #pragma pack(pop)

    if (ctx->GetSystemCpuSetInformation) {
        // get buffer size needed
        u32 buffer_size = 0;
        ctx->GetSystemCpuSetInformation(0, 0, &buffer_size, 0, 0);
        // allocate buffer and get data
        u8* buffer = (u8*)ctx->GlobalAlloc(mt_GPTR, buffer_size);
        u8* current_chunk = buffer;
        if (buffer && ctx->GetSystemCpuSetInformation(buffer, buffer_size, &buffer_size, 0, 0)) {
            // simply divide the returned cores into two
            // efficiency classes. this assumes that only two
            // efficiency classes exist, which is true for now.
            u32 efficiency_0_cores = 0;
            u32 efficiency_1_cores = 0;
            u32 last_core_seen     = 0xffffffff;
            // loop through data
            for (u32 processed_data = 0; processed_data < buffer_size; ) {
                // get current chunk which is one mt_cpu_set_information struct
                mt_cpu_set_information* data = (mt_cpu_set_information*)current_chunk;
                if (data->type == mt_cpu_set_information_type) {
                    // every struct instance is always a logical processor
                    logical_procs++;
                    // we're counting distinct core_idx values; they're returned in order
                    if (last_core_seen != data->cpu_set.core_idx) {
                        processor_cores++;
                        // divide cores into two efficiency classes
                        if (data->cpu_set.efficiency_class == 0) {
                            efficiency_0_cores++;
                        } else {
                            efficiency_1_cores++;
                        }
                    }
                    last_core_seen = data->cpu_set.core_idx;
                    #if _MULTITHREAD_DEBUG
                    printf("code %d: group %3d, logical %3d, core %3d, numa %3d, eff %3d\n", 
                            data->cpu_set.id, data->cpu_set.group, data->cpu_set.logical_proc_idx, 
                            data->cpu_set.core_idx, data->cpu_set.numa_node_idx, data->cpu_set.efficiency_class);
                    #endif
                }
                processed_data += data->size;
                current_chunk += data->size;
            }
            // if we have any efficiency class 1 cores, we remember the low-efficiency core count
            // (on a CPU with no e-cores, all cores are efficiency class 0)
            if (efficiency_1_cores > 0)
                efficient_cores = efficiency_0_cores;
            // we're done with the buffer
            ctx->GlobalFree(buffer);
        }
    } else {
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
        ctx->GlobalFree(buffer);
    }

    #if _MULTITHREAD_DEBUG
        printf("cores:     %3d\n", processor_cores);
        printf("threads:   %3d\n", logical_procs);
        printf("active:    %3d\n", GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
        printf("efficient: %3d\n", efficient_cores);
    #endif
    return processor_cores - efficient_cores;
}

#if _MULTITHREAD_DEBUG
void __stdcall testworker(ptr param, i32 thread_idx) {
    mt_ctx *ctx = (mt_ctx*)param;
    u64 now, freq;
    ctx->QueryPerformanceCounter(&now);
    ctx->QueryPerformanceFrequency(&freq);
    now = now * 10000000 / freq;
    //printf("%3.3d: %llu\n", thread_idx, now);
}

void main() {
    mt_ctx* ctx = mt_init(0);
    if (ctx) {
        for (int i=0; i<5; i++) {
            mt_run(ctx, testworker, ctx);
        }
        mt_deinit(ctx);
    }
}
#endif
