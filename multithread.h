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

typedef void (__stdcall *mt_worker_thread_t)(ptr);

#define mt_GPTR 0x40

#ifndef _BASIC_KERNEL_DEFINED
#define _BASIC_KERNEL_DEFINED
typedef ptr (*LoadLibraryA_t)(char *name);
typedef void (*FreeLibrary_t)(ptr modulehandle);
typedef ptr (*GlobalAlloc_t)(u32 flags, u32 size);
typedef u32 (*GlobalFree_t)(ptr ptr);
#endif

typedef ptr (*CreateEventA_t)(ptr, u32, u32, i8*);
typedef u32 (*CloseHandle_t)(ptr);
typedef u32 (*SetEvent_t)(ptr);
typedef u32 (*ResetEvent_t)(ptr);
typedef u32 (*WaitForSingleObject_t)(ptr, u32);
typedef u32 (*WaitForMultipleObjects_t)(u32, ptr, u32, u32);
typedef ptr (*CreateThread_t)(ptr, u32, ptr, ptr, u32, ptr);
typedef u32 (*ExitThread_t)(u32);
typedef u32 (*Sleep_t)(u32);
typedef u32 (*InitializeThreadpoolEnvironment_t)(ptr);
typedef ptr (*CreateThreadpool_t)(ptr);
typedef u32 (*SetThreadpoolThreadMaximum_t)(ptr, u32);
typedef u32 (*SetThreadpoolThreadMinimum_t)(ptr, u32);
typedef ptr (*CreateThreadpoolWork_t)(ptr, ptr, ptr);
typedef u32 (*SubmitThreadpoolWork_t)(ptr);
typedef ptr (*CreateThreadpoolWait_t)(ptr, ptr, ptr);
typedef u32 (*SetThreadpoolWait_t)(ptr, ptr, ptr);
typedef u32 (*WaitForThreadpoolWorkCallbacks_t)(ptr, u32);
typedef u32 (*WaitForThreadpoolWaitCallbacks_t)(ptr, u32);
typedef u32 (*CloseThreadpoolWait_t)(ptr);
typedef u32 (*CloseThreadpoolWork_t)(ptr);
typedef u32 (*CloseThreadpool_t)(ptr);
typedef u32 (*GetLogicalProcessorInformation_t)(ptr, ptr);
typedef u32 (*GetLastError_t)(void);
typedef u32 (*QueryPerformanceCounter_t)(ptr);
typedef u32 (*QueryPerformanceFrequency_t)(ptr);
#if !defined(GetProcAddress_t_defined)
typedef ptr (*GetProcAddress_t)(ptr, i8*);
#endif

typedef struct _mt_ctx mt_ctx;
typedef u32 (*mt_run_threads_t)(mt_ctx*, mt_worker_thread_t, ptr);
mt_ctx* mt_init_ctx(u32 num_threads);
void mt_deinit_ctx(mt_ctx* ctx);
u32 mt_run_threads(mt_ctx* ctx, mt_worker_thread_t worker, ptr param);

typedef struct _mt_ctx {
    ptr kernel32;
    GetProcAddress_t                    GetProcAddress;
    LoadLibraryA_t                      LoadLibraryA;
    FreeLibrary_t                       FreeLibrary;
    GlobalAlloc_t                       GlobalAlloc;
    GlobalFree_t                        GlobalFree;
    CreateEventA_t                      CreateEventA;
    CloseHandle_t                       CloseHandle;
    SetEvent_t                          SetEvent;
    ResetEvent_t                        ResetEvent;
    WaitForSingleObject_t               WaitForSingleObject;
    WaitForMultipleObjects_t            WaitForMultipleObjects;
    InitializeThreadpoolEnvironment_t   InitializeThreadpoolEnvironment;
    CreateThreadpool_t                  CreateThreadpool;
    SetThreadpoolThreadMaximum_t        SetThreadpoolThreadMaximum;
    SetThreadpoolThreadMinimum_t        SetThreadpoolThreadMinimum;
    CreateThreadpoolWork_t              CreateThreadpoolWork;
    SubmitThreadpoolWork_t              SubmitThreadpoolWork;
    CreateThreadpoolWait_t              CreateThreadpoolWait;
    SetThreadpoolWait_t                 SetThreadpoolWait;
    WaitForThreadpoolWaitCallbacks_t    WaitForThreadpoolWaitCallbacks;
    WaitForThreadpoolWorkCallbacks_t    WaitForThreadpoolWorkCallbacks;
    CloseThreadpoolWait_t               CloseThreadpoolWait;
    CloseThreadpoolWork_t               CloseThreadpoolWork;
    CloseThreadpool_t                   CloseThreadpool;
    GetLogicalProcessorInformation_t    GetLogicalProcessorInformation;
    GetLastError_t                      GetLastError;
    QueryPerformanceCounter_t           QueryPerformanceCounter;
    QueryPerformanceFrequency_t         QueryPerformanceFrequency;
    ptr                                 cbe;
    ptr                                 pool;
    u32                                 num_threads;
    mt_run_threads_t                    mt_run_threads;
} mt_ctx;
