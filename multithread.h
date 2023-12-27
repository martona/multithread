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
#define mt_WAIT_OBJECT_0 0x00000000L

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
typedef ptr (*CreateThread_t)(ptr, u32, ptr, ptr, u32, ptr);
typedef u32 (*WaitForSingleObject_t)(ptr, u32);
typedef ptr (*CreateEventA_t)(ptr, u32, u32, ptr);
typedef u32 (*SetEvent_t)(ptr);
typedef u32 (*ResetEvent_t)(ptr);
typedef u32 (*CloseHandle_t)(ptr);
typedef u32 (*WaitForMultipleObjects_t)(u32, ptr, u32, u32);

#if !defined(GetProcAddress_t_defined)
typedef ptr (*GetProcAddress_t)(ptr, i8*);
#endif

// define ptr types if someone's only including the .h
typedef struct _mt_ctx mt_ctx;
typedef void (__stdcall *mt_client_worker_t)(ptr, i32);
typedef u32 (*mt_run_t)(mt_ctx*, mt_client_worker_t, ptr);
// forward defines
mt_ctx* mt_init(u32 num_threads);
void mt_deinit(mt_ctx* ctx);
u32 mt_run(mt_ctx* ctx, mt_client_worker_t worker, ptr param);

// the bigol' context structure that holds function pointers
// as well as the thread pool state
typedef struct _mt_ctx {
    ptr kernel32;
    GetProcAddress_t                    GetProcAddress;
    LoadLibraryA_t                      LoadLibraryA;
    FreeLibrary_t                       FreeLibrary;
    GlobalAlloc_t                       GlobalAlloc;
    GlobalFree_t                        GlobalFree;
    GetLastError_t                      GetLastError;
    QueryPerformanceCounter_t           QueryPerformanceCounter;
    QueryPerformanceFrequency_t         QueryPerformanceFrequency;
    GetLogicalProcessorInformation_t    GetLogicalProcessorInformation;
    #if defined(_MULTITHREAD_MSPOOL_IMPL)
    InitializeThreadpoolEnvironment_t   InitializeThreadpoolEnvironment;
    CreateThreadpool_t                  CreateThreadpool;
    SetThreadpoolThreadMaximum_t        SetThreadpoolThreadMaximum;
    SetThreadpoolThreadMinimum_t        SetThreadpoolThreadMinimum;
    CreateThreadpoolWork_t              CreateThreadpoolWork;
    SubmitThreadpoolWork_t              SubmitThreadpoolWork;
    WaitForThreadpoolWorkCallbacks_t    WaitForThreadpoolWorkCallbacks;
    CloseThreadpoolWork_t               CloseThreadpoolWork;
    CloseThreadpool_t                   CloseThreadpool;
    ptr                                 cbe;
    ptr                                 pool;
    #elif defined(_MULTITHREAD_GOOD_IMPL)
    CreateThread_t                      CreateThread;
    WaitForSingleObject_t               WaitForSingleObject;
    CreateEventA_t                      CreateEventA;
    SetEvent_t                          SetEvent;
    ResetEvent_t                        ResetEvent;
    CloseHandle_t                       CloseHandle;
    WaitForMultipleObjects_t            WaitForMultipleObjects;
    ptr*                                ahThreadsStart;                     // array of event handles that start the inividual threads
    ptr                                 hThreadsExit;                       // a single manual-reset event that exits all threads
    ptr*                                ahThreadsDone;                      // array of handles to all threads' done events
    ptr*                                ahThreads;                          // array of handles to all threads
    ptr                                 pThreadContexts;                    // a blob of thread contexts
    mt_client_worker_t                  worker;                             // the worker function
    ptr                                 param;                              // the parameter to pass to the worker function
    #endif
    u32                                 num_threads;
    i32                                 thread_idx;
    mt_run_t                            mt_run;
} mt_ctx;
