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
bool (*fit_inc)(vector_type<vector_type<double>> X, vector_type<int64_t> y) = nullptr;

#include "../server/monetdb_conn.h"
__AQEXPORT__(int) action(Context* cxt) {
	using namespace std;
	using namespace types;
    if (fit_inc == nullptr)
        fit_inc = (decltype(fit_inc))(cxt->get_module_function("fit_inc"));

	auto server = static_cast<DataSource*>(cxt->alt_server);
    auto len = uint32_t(monetdbe_get_size(*((void**)server->server), "source"));
    auto x_1bN = ColRef<vector_type<double>>(len, monetdbe_get_col(*((void**)(server->server)), "source", 0));
    auto y_6uX = ColRef<int64_t>(len, monetdbe_get_col(*((void**)(server->server)), "source", 1));
    fit_inc(x_1bN, y_6uX);
    puts("action done.");
    return 0;
}
