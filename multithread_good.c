#if defined(_MULTITHREAD_GOOD_IMPL)
#include "multithread.h"
#include "submodules/getprocaddress/getprocaddress.c"
#include "multithread_base.c"

//-------------------------------------------------------------------------------------
// outer worker thread

typedef struct {
    mt_ctx* ctx;                // the context
    u32     thread_idx;         // the thread's index
} mt_thread_ctx;

// our worker that calls the user's worker when needed
static u32 __stdcall mt_threadproc(void* p) {
    mt_thread_ctx* ctx = (mt_thread_ctx*)p;
    while (1) {
        // increment the count of threads that are about to wait
        __atomic_fetch_add(&ctx->ctx->cnt_threads_ready, 1, __ATOMIC_SEQ_CST);
        u32 signal = ctx->ctx->WaitForMultipleObjects(2, &ctx->ctx->hThreadsStart, 0, -1);
        if (signal != mt_WAIT_OBJECT_0) {
            return 0;
        }
        mt_client_worker_t worker = ctx->ctx->worker;
        worker(ctx->ctx->param);
        ctx->ctx->SetEvent(ctx->ctx->ahThreadsDone[ctx->thread_idx]);
        ctx->ctx->WaitForSingleObject(ctx->ctx->hThreadsReset, -1);
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////
// context to be supplied to all calls

mt_ctx* mt_init(u32 num_threads) {
    ptr kernel32 = gpa_getkernel32();
    if (kernel32) {
        GetProcAddress_t GetProcAddress = (GetProcAddress_t)gpa_getgetprocaddress(kernel32);
        GlobalFree_t GlobalFree = (GlobalFree_t)GetProcAddress(kernel32, "GlobalFree");
        GlobalAlloc_t GlobalAlloc = (GlobalAlloc_t)GetProcAddress(kernel32, "GlobalAlloc");
        mt_ctx* ctx = GlobalAlloc(mt_GPTR, sizeof(mt_ctx));
        if (ctx) {
            ctx->mt_run                         = mt_run;
            ctx->kernel32                       = kernel32;
            ctx->GlobalAlloc                    = GlobalAlloc;
            ctx->GetProcAddress                 = GetProcAddress;
            ctx->GlobalFree                     = GlobalFree;
            ctx->LoadLibraryA                   = (LoadLibraryA_t)                  GetProcAddress(kernel32, "LoadLibraryA");
            ctx->FreeLibrary                    = (FreeLibrary_t)                   GetProcAddress(kernel32, "FreeLibrary");
            ctx->GetLogicalProcessorInformation = (GetLogicalProcessorInformation_t)GetProcAddress(kernel32, "GetLogicalProcessorInformation");
            ctx->GetLastError                   = (GetLastError_t)                  GetProcAddress(kernel32, "GetLastError");
            ctx->QueryPerformanceCounter        = (QueryPerformanceCounter_t)       GetProcAddress(kernel32, "QueryPerformanceCounter");
            ctx->QueryPerformanceFrequency      = (QueryPerformanceFrequency_t)     GetProcAddress(kernel32, "QueryPerformanceFrequency");
            ctx->CreateThread                   = (CreateThread_t)                  GetProcAddress(kernel32, "CreateThread");
            ctx->WaitForSingleObject            = (WaitForSingleObject_t)           GetProcAddress(kernel32, "WaitForSingleObject");
            ctx->CreateEventA                   = (CreateEventA_t)                  GetProcAddress(kernel32, "CreateEventA");
            ctx->SetEvent                       = (SetEvent_t)                      GetProcAddress(kernel32, "SetEvent");
            ctx->ResetEvent                     = (ResetEvent_t)                    GetProcAddress(kernel32, "ResetEvent");
            ctx->CloseHandle                    = (CloseHandle_t)                   GetProcAddress(kernel32, "CloseHandle");
            ctx->WaitForMultipleObjects         = (WaitForMultipleObjects_t)        GetProcAddress(kernel32, "WaitForMultipleObjects");
            ctx->hThreadsStart                  = ctx->CreateEventA(0, 1, 0, 0);    // manual reset, not signaled
            ctx->hThreadsExit                   = ctx->CreateEventA(0, 1, 0, 0);    // manual reset, not signaled
            ctx->hThreadsReset                  = ctx->CreateEventA(0, 1, 0, 0);    // manual reset, not signaled
            ctx->num_threads                    = num_threads ? num_threads : mt_get_cputhreads(ctx);
            ctx->ahThreads                      = GlobalAlloc(mt_GPTR, ctx->num_threads * sizeof(ptr));
            ctx->ahThreadsDone                  = GlobalAlloc(mt_GPTR, ctx->num_threads * sizeof(ptr));
            ctx->pThreadContexts                = GlobalAlloc(mt_GPTR, ctx->num_threads * sizeof(mt_thread_ctx));
            ctx->worker                         = 0;
            ctx->param                          = 0;
            ctx->cnt_threads_ready              = 0;
            if (ctx->ahThreads && ctx->ahThreadsDone && ctx->pThreadContexts) {
                for (u32 i=0; i<ctx->num_threads; i++) {
                    ctx->ahThreadsDone[i] = ctx->CreateEventA(0, 0, 0, 0);    // auto reset, not signaled
                    ((mt_thread_ctx*)ctx->pThreadContexts)[i].ctx = ctx;
                    ((mt_thread_ctx*)ctx->pThreadContexts)[i].thread_idx = i;
                    ctx->ahThreads[i] = ctx->CreateThread(0, 0, mt_threadproc, &((mt_thread_ctx*)ctx->pThreadContexts)[i], 0, 0);
                }
                return ctx;
            }
            // we failed to allocate something, so clean up
            ctx->GlobalFree(ctx->pThreadContexts);
            ctx->GlobalFree(ctx->ahThreadsDone);
            ctx->GlobalFree(ctx->ahThreads);
            ctx->CloseHandle(ctx->hThreadsStart);
            ctx->CloseHandle(ctx->hThreadsReset);
            ctx->CloseHandle(ctx->hThreadsExit);
            GlobalFree(ctx);
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// free a context and all associated resources

void mt_deinit(mt_ctx* ctx) {
    if (ctx) {
        ctx->SetEvent(ctx->hThreadsExit);
        ctx->WaitForMultipleObjects(ctx->num_threads, ctx->ahThreads, 1, -1);
        ctx->CloseHandle(ctx->hThreadsStart);
        ctx->CloseHandle(ctx->hThreadsReset);
        ctx->CloseHandle(ctx->hThreadsExit);
        for (u32 i=0; i<ctx->num_threads; i++) {
            ctx->CloseHandle(ctx->ahThreads[i]);
            ctx->CloseHandle(ctx->ahThreadsDone[i]);
        }
        ctx->GlobalFree(ctx->pThreadContexts);
        ctx->GlobalFree(ctx->ahThreadsDone);
        ctx->GlobalFree(ctx->ahThreads);
        ctx->GlobalFree(ctx);
    }   
}


///////////////////////////////////////////////////////////////////////////////////////
// run threads with supplied parameters, return when all are done

u32 mt_run(mt_ctx* ctx, mt_client_worker_t worker, ptr param) {
    if (ctx) {
        __atomic_store_n(&ctx->worker, worker, __ATOMIC_SEQ_CST);
        __atomic_store_n(&ctx->param, param, __ATOMIC_SEQ_CST);
        // signal the threads to start
        ctx->SetEvent(ctx->hThreadsStart);
        // wait for all threads to finish and wait for the reset event
        ctx->WaitForMultipleObjects(ctx->num_threads, ctx->ahThreadsDone, 1, -1);
        // now is a safe time to reset the start event
        ctx->ResetEvent(ctx->hThreadsStart);
        // reset the count of threads that are about to wait at the start
        __atomic_store_n(&ctx->cnt_threads_ready, 0, __ATOMIC_SEQ_CST);
        // cause the threads to restart their loop, each incrementing the above counter
        ctx->SetEvent(ctx->hThreadsReset);
        u32 cnt = 0;
        while (cnt < ctx->num_threads)
            __atomic_load(&ctx->cnt_threads_ready, &cnt, __ATOMIC_SEQ_CST);
        // all threads are waiting/about to wait to start, reset the reset event
        ctx->ResetEvent(ctx->hThreadsReset);

        return 1;
    }
    //no succ!
    return 0;
}

#if _MULTITHREAD_DEBUG
void __stdcall testworker(ptr param) {
    mt_ctx *ctx = (mt_ctx*)param;
    u64 now, freq;
    ctx->QueryPerformanceCounter(&now);
    ctx->QueryPerformanceFrequency(&freq);
    now = now * 10000000 / freq;
    printf("%llu\n", now);
}

int main() {
    mt_ctx* ctx = mt_init(0);
    if (ctx) {
        for (int i=0; i<5; i++) {
            mt_run(ctx, testworker, ctx);
        }
        mt_deinit(ctx);
    }
    return 0;
}
#endif
#endif // defined(_MULTITHREAD_GOOD_IMPL)