#include "../server/libaquery.h"

#ifndef __AQ_USE_THREADEDGC__

#include "../server/gc.h"
__AQEXPORT__(void) __AQ_Init_GC__(Context* cxt) {
    GC::gc_handle = static_cast<GC*>(cxt->gc);
    GC::scratch_space = nullptr;
}

#else // __AQ_USE_THREADEDGC__
#define __AQ_Init_GC__(x)
#endif // __AQ_USE_THREADEDGC__

#include "../server/monetdb_conn.h"
__AQEXPORT__(int) query(Context* cxt) {
	using namespace std;
	using namespace types;
    auto server = static_cast<Server*>(cxt->alt_server);
    static uint32_t old_sz = 0;
    constexpr static uint32_t min_delta = 200;
    auto newsz = monetdbe_get_size(*(void**) server->server, "source");
    if (newsz > old_sz + min_delta) {
        puts("query true.");
        old_sz = uint32_t(newsz);
        return 1;
    }
    puts("query false.");
    return 0;
}
