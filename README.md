
# multithread
A self-contained multithreaded framework for executing in an environmnent without any linked libraries (such as the CRT) or any knowledge of the environment (such as the addresses of any Win32 API calls).

# but why
I am/was working on an image library for AutoHotkey and wanted my C code to have access to an easy-to-use threadpool. 

# how
`#include "multithread_good.c"`in your source file and you're good to go. If you need the datatype declarations in other source files you can `#include multithread.h"` there.

You could use `multithread_mspool.c` instead but it's a strictly worse version, at least for the "fire off `n`threads as quickly as possible" scenario. 

Call `mr_ctx* mt_init(u32 num_threads)`and pass in zero if you don't know how many threads you want; the code will use the number of cores avilable.

Define your worker something like this:
```
void __stdcall testworker(ptr param, i32 thread_idx) {
	// actual work goes here
}
```
`param` is your user-supplied pointer, thread_idx is a number between `0`and `n` where `n` is the maximum number of workers spawned. You can use it to easily divide up work (such as a block of scanlines).

You pass the worker's address to `u32 mt_run(mt_ctx*  ctx, mt_client_worker_t  worker, ptr param)` where `ctx` is the context you got  from `mt_init` and `param` is whatever you're passing to the workers. `mt_run` will return when all your workers have finished. You can then call `mt_run` again, or call `mt_deinit(mt_ctx* ctx)` when you're finished which cleans up everything and invalidates the `ctx` pointer.
