/*
    see readme.md for some (thin) details
    (C) 2023 https://github.com/martona/multithread
    MIT License
*/
#if defined(_MULTITHREAD_GOOD_IMPL)

//-------------------------------------------------------------------------------------
// outer worker thread

// our worker that calls the user's worker when needed
static u32 __stdcall mt_threadproc(void* p) {
    mt_ctx* ctx = (mt_ctx*)p;
    i32 thread_idx = __atomic_fetch_add(&ctx->thread_idx, 1, __ATOMIC_SEQ_CST);
    ptr wait_handles[2] = { ctx->ahThreadsStart[thread_idx], ctx->hThreadsExit };
    while (1) {
        if (ctx->WaitForMultipleObjects(2, wait_handles, 0, -1) != mt_WAIT_OBJECT_0)
            return 0;
        mt_client_worker_t worker = ctx->worker;
        worker(ctx->param, thread_idx);
        ctx->SetEvent(ctx->ahThreadsDone[thread_idx]);
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
            ctx->GetSystemCpuSetInformation     = (GetSystemCpuSetInformation_t)    GetProcAddress(kernel32, "GetSystemCpuSetInformation");
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
            ctx->num_threads                    = num_threads ? num_threads : mt_get_cputhreads(ctx);
            ctx->ahThreads                      = GlobalAlloc(mt_GPTR, ctx->num_threads * sizeof(ptr));
            ctx->ahThreadsStart                 = GlobalAlloc(mt_GPTR, ctx->num_threads * sizeof(ptr));
            ctx->ahThreadsDone                  = GlobalAlloc(mt_GPTR, ctx->num_threads * sizeof(ptr));
            ctx->hThreadsExit                   = ctx->CreateEventA(0, 1, 0, 0);    // manual reset
            ctx->worker                         = 0;
            ctx->param                          = 0;
            ctx->thread_idx                     = 0;
            if (ctx->ahThreads && ctx->ahThreadsStart && ctx->ahThreadsDone) {
                for (u32 i=0; i<ctx->num_threads; i++) {
                    ctx->ahThreadsDone[i]  = ctx->CreateEventA(0, 0, 0, 0);    // auto reset, not signaled
                    ctx->ahThreadsStart[i] = ctx->CreateEventA(0, 0, 0, 0);    // auto reset, not signaled
                    ctx->ahThreads[i] = ctx->CreateThread(0, 0, mt_threadproc, ctx, 0, 0);
                }
                return ctx;
            }
            // we failed to allocate something, so clean up
            ctx->CloseHandle(ctx->ahThreadsStart);
            ctx->GlobalFree(ctx->ahThreadsDone);
            ctx->GlobalFree(ctx->ahThreads);
            ctx->GlobalFree(ctx->pThreadContexts);
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
        ctx->CloseHandle(ctx->hThreadsExit);
        for (u32 i=0; i<ctx->num_threads; i++) {
            ctx->CloseHandle(ctx->ahThreads[i]);
            ctx->CloseHandle(ctx->ahThreadsDone[i]);
            ctx->CloseHandle(ctx->ahThreadsStart[i]);
        }
        ctx->GlobalFree(ctx->ahThreadsDone);
        ctx->GlobalFree(ctx->ahThreadsStart);
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
        for (u32 i=0; i<ctx->num_threads; i++)
            ctx->SetEvent(ctx->ahThreadsStart[i]);
        // wait for all threads to finish their work
        ctx->WaitForMultipleObjects(ctx->num_threads, ctx->ahThreadsDone, 1, -1);
        // it's that easy
        return 1;
    }
    //no succ!
    return 0;
}

#endif // defined(_MULTITHREAD_GOOD_IMPL)