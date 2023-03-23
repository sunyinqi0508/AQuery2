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
#include "../csv.h"

__AQEXPORT__(int) ld(Context* cxt) {
	using namespace std;
	using namespace types;
	static int cnt = 0;
	if (cnt > 700)
		return 1;
	else
		++cnt;
	char data_name[] = "data/electricity/electricity       ";
	auto server = static_cast<DataSource*>(cxt->alt_server);
	const char* names_fZrv[] = {"x", "y"};
	auto tbl_6erF = new TableInfo<vector_type<double>,int64_t>("source", names_fZrv);
	decltype(auto) c_31ju0e = tbl_6erF->get_col<0>();
	decltype(auto) c_4VlzrR = tbl_6erF->get_col<1>();
	c_31ju0e.init("x");
	c_4VlzrR.init("y");
	auto nxt = to_text(data_name + 28, cnt);
	memcpy(nxt, ".csv", 5);
	puts(data_name);
	AQCSVReader<2, ',', ';'> csv_reader_7g0GY7(data_name);
	csv_reader_7g0GY7.next_line();
	vector_type<double> tmp_5XMNcBz5;
	int64_t tmp_5dAHIJ1d;
	while(csv_reader_7g0GY7.read_row(tmp_5XMNcBz5,tmp_5dAHIJ1d)) { 
		c_31ju0e.emplace_back(tmp_5XMNcBz5);
		c_4VlzrR.emplace_back(tmp_5dAHIJ1d);
	}
	tbl_6erF->monetdb_append_table(cxt->alt_server, "source");
	return 0;
}

