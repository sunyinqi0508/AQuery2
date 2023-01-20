#include "./server/libaquery.h"

#ifndef __AQ_USE_THREADEDGC__

#include "./server/gc.h"
__AQEXPORT__(void) __AQ_Init_GC__(Context* cxt) {
    GC::gc_handle = static_cast<GC*>(cxt->gc);
    GC::scratch_space = nullptr;
}

#else // __AQ_USE_THREADEDGC__
#define __AQ_Init_GC__(x)
#endif // __AQ_USE_THREADEDGC__
