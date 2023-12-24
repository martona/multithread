#pragma once

#if !defined(_BASIC_TYPES_DEFINED)
#define _BASIC_TYPES_DEFINED
typedef char                 i8;
typedef short               i16;
typedef int                 i32;
typedef long long           i64;
typedef unsigned char        u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef void*               ptr;
typedef unsigned short      wchar;
#endif

#define mt_GPTR 0x40

#ifndef _BASIC_KERNEL_DEFINED
#define _BASIC_KERNEL_DEFINED
typedef ptr (*LoadLibraryA_t)(char *name);
typedef void (*FreeLibrary_t)(ptr modulehandle);
typedef ptr (*GlobalAlloc_t)(u32 flags, u32 size);
typedef u32 (*GlobalFree_t)(ptr ptr);
#endif

typedef u32 (*InitializeThreadpoolEnvironment_t)(ptr);
typedef ptr (*CreateThreadpool_t)(ptr);
typedef u32 (*SetThreadpoolThreadMaximum_t)(ptr, u32);
typedef u32 (*SetThreadpoolThreadMinimum_t)(ptr, u32);
typedef ptr (*CreateThreadpoolWork_t)(ptr, ptr, ptr);
typedef u32 (*SubmitThreadpoolWork_t)(ptr);
typedef u32 (*WaitForThreadpoolWorkCallbacks_t)(ptr, u32);
typedef u32 (*CloseThreadpoolWork_t)(ptr);
typedef u32 (*CloseThreadpool_t)(ptr);
typedef u32 (*GetLogicalProcessorInformation_t)(ptr, ptr);
typedef u32 (*GetLastError_t)(void);
typedef u32 (*QueryPerformanceCounter_t)(ptr);
typedef u32 (*QueryPerformanceFrequency_t)(ptr);
#if !defined(GetProcAddress_t_defined)
typedef ptr (*GetProcAddress_t)(ptr, i8*);
#endif

// define ptr types if someone's only including the .h
typedef struct _mt_ctx mt_ctx;
typedef void (__stdcall *mt_worker_t)(ptr);
typedef u32 (*mt_run_t)(mt_ctx*, mt_worker_t, ptr);
mt_ctx* mt_init(u32 num_threads);
void mt_deinit(mt_ctx* ctx);
u32 mt_run(mt_ctx* ctx, mt_worker_t worker, ptr param);

// the bigol' context structure that holds function pointers
// as well as the thread pool state
typedef struct _mt_ctx {
    ptr kernel32;
    GetProcAddress_t                    GetProcAddress;
    LoadLibraryA_t                      LoadLibraryA;
    FreeLibrary_t                       FreeLibrary;
    GlobalAlloc_t                       GlobalAlloc;
    GlobalFree_t                        GlobalFree;
    InitializeThreadpoolEnvironment_t   InitializeThreadpoolEnvironment;
    CreateThreadpool_t                  CreateThreadpool;
    SetThreadpoolThreadMaximum_t        SetThreadpoolThreadMaximum;
    SetThreadpoolThreadMinimum_t        SetThreadpoolThreadMinimum;
    CreateThreadpoolWork_t              CreateThreadpoolWork;
    SubmitThreadpoolWork_t              SubmitThreadpoolWork;
    WaitForThreadpoolWorkCallbacks_t    WaitForThreadpoolWorkCallbacks;
    CloseThreadpoolWork_t               CloseThreadpoolWork;
    CloseThreadpool_t                   CloseThreadpool;
    GetLogicalProcessorInformation_t    GetLogicalProcessorInformation;
    GetLastError_t                      GetLastError;
    QueryPerformanceCounter_t           QueryPerformanceCounter;
    QueryPerformanceFrequency_t         QueryPerformanceFrequency;
    ptr                                 cbe;
    ptr                                 pool;
    u32                                 num_threads;
    mt_run_t                            mt_run;
} mt_ctx;
