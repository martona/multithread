#if defined(_MULTITHREAD_MSPOOL_IMPL)
#include "multithread.h"
#include "submodules/getprocaddress/getprocaddress.c"
#include "multithread_base.c"

//-------------------------------------------------------------------------------------
// data types required for mt_init

#define mt_TP_CALLBACK_PRIORITY_NORMAL 1

typedef struct {
    u32         version;
    ptr         pool;
    ptr         cleanupgroup;
    ptr         cleanupgroupcancelcallback;
    ptr         raceDll;
    ptr         activationcontext;
    ptr         finalizationcallback;
    u32         flags;
    u32         callbackpriority;
    u32         size;
} mt_CallbackEnvironment;

static inline u32 mt_InitializeThreadpoolEnvironment(void* p) {
    // since this came from GlobalAlloc(GPTR,...) it's already zeroed
    mt_CallbackEnvironment* e       = (mt_CallbackEnvironment*)p;
    e->version                      = 3;
    e->callbackpriority             = mt_TP_CALLBACK_PRIORITY_NORMAL;
    e->size                         = sizeof(mt_CallbackEnvironment);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////
// context to be supplied to all other calls

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
            ctx->InitializeThreadpoolEnvironment= mt_InitializeThreadpoolEnvironment;
            ctx->CreateThreadpool               = (CreateThreadpool_t)              GetProcAddress(kernel32, "CreateThreadpool");
            ctx->SetThreadpoolThreadMaximum     = (SetThreadpoolThreadMaximum_t)    GetProcAddress(kernel32, "SetThreadpoolThreadMaximum");
            ctx->SetThreadpoolThreadMinimum     = (SetThreadpoolThreadMinimum_t)    GetProcAddress(kernel32, "SetThreadpoolThreadMinimum");
            ctx->CreateThreadpoolWork           = (CreateThreadpoolWork_t)          GetProcAddress(kernel32, "CreateThreadpoolWork");
            ctx->SubmitThreadpoolWork           = (SubmitThreadpoolWork_t)          GetProcAddress(kernel32, "SubmitThreadpoolWork");
            ctx->WaitForThreadpoolWorkCallbacks = (WaitForThreadpoolWorkCallbacks_t)GetProcAddress(kernel32, "WaitForThreadpoolWorkCallbacks");
            ctx->CloseThreadpoolWork            = (CloseThreadpoolWork_t)           GetProcAddress(kernel32, "CloseThreadpoolWork");
            ctx->CloseThreadpool                = (CloseThreadpool_t)               GetProcAddress(kernel32, "CloseThreadpool");
            ctx->GetLogicalProcessorInformation = (GetLogicalProcessorInformation_t)GetProcAddress(kernel32, "GetLogicalProcessorInformation");
            ctx->GetLastError                   = (GetLastError_t)                  GetProcAddress(kernel32, "GetLastError");
            ctx->QueryPerformanceCounter        = (QueryPerformanceCounter_t)       GetProcAddress(kernel32, "QueryPerformanceCounter");
            ctx->QueryPerformanceFrequency      = (QueryPerformanceFrequency_t)     GetProcAddress(kernel32, "QueryPerformanceFrequency");
            ctx->cbe                            = GlobalAlloc(mt_GPTR, sizeof(mt_CallbackEnvironment));
            if (ctx->cbe) {
                ctx->InitializeThreadpoolEnvironment(ctx->cbe);
                ctx->pool = ctx->CreateThreadpool(0);
                if (ctx->pool) {
                    ctx->num_threads = num_threads ? num_threads : mt_get_cputhreads(ctx);
                    ctx->thread_idx = 0;
                    ctx->SetThreadpoolThreadMaximum(ctx->pool, ctx->num_threads);
                    ctx->SetThreadpoolThreadMinimum(ctx->pool, ctx->num_threads);
                    ((mt_CallbackEnvironment*)(ctx->cbe))->pool = ctx->pool;
                    return ctx;
                }
                ctx->GlobalFree(ctx->cbe);
            }
            GlobalFree(ctx);
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// free a context and all associated resources

void mt_deinit(mt_ctx* ctx) {
    if (ctx) {
        ctx->CloseThreadpool(ctx->pool);
        ctx->GlobalFree(ctx);
    }   
}


//-------------------------------------------------------------------------------------
// data type required for mt_run

// outer worker thread paramters
typedef struct {
    mt_ctx* ctx;                        // the context
    mt_client_worker_t worker_thread;   // the inner worker thread function
    ptr param;                          // the parameter to pass on
} mt_thread_ctx;

//-------------------------------------------------------------------------------------
// outer worker thread

// worker wrapper for the simpler client thread
static void __stdcall mt_work_callback(ptr instance, mt_thread_ctx* ctx, ptr work) {
    ctx->worker_thread(ctx->param, -1);
}

///////////////////////////////////////////////////////////////////////////////////////
// run threads with supplied parameters, return when all are done

u32 mt_run(mt_ctx* ctx, mt_client_worker_t worker, ptr param) {
    if (ctx) {
        //package input into a struct for the callbacks
        mt_thread_ctx t = {ctx, worker, param};
        //create a work item for the thread pool
        ptr work = ctx->CreateThreadpoolWork(mt_work_callback, &t, ctx->cbe);
        if (work) {
            //submit it as many times as we have threads
            for (u32 i=0; i<ctx->num_threads; i++)
                ctx->SubmitThreadpoolWork(work);
            //at this point they're all running, so wait for them to finish
            ctx->WaitForThreadpoolWorkCallbacks(work, 0);
            //close the work object
            ctx->CloseThreadpoolWork(work);
            //succ!
            return 1;
        }
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
#endif //_MULTITHREAD_MSPOOL_IMPL
