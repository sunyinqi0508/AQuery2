#include "./server/libaquery.h"

#ifndef __AQ_USE_THREADEDGC__

#include "./server/gc.h"
__AQEXPORT__(void) __AQ_Init_GC__(Context* cxt) {
    GC::gc_handle = static_cast<GC*>(cxt->gc);
}

#else // __AQ_USE_THREADEDGC__
#define __AQ_Init_GC__(x)
#endif // __AQ_USE_THREADEDGC__
#include "./server/hasher.h"
#include "./server/monetdb_conn.h"
#include "./server/aggregations.h"

__AQEXPORT__(int) dll_2Cxoox(Context* cxt) {
	using namespace std;
	using namespace types;
	auto server = static_cast<Server*>(cxt->alt_server);
auto len_4ycjiV = server->cnt;
auto mont_8AE = ColRef<const char*>(len_4ycjiV, server->getCol(0));
auto sales_2RB = ColRef<int>(len_4ycjiV, server->getCol(1));
const char* names_6pIt[] = {"mont", "minw2ysales"};
auto out_2LuaMH = new TableInfo<const char*,vector_type<double>>("out_2LuaMH", names_6pIt);
decltype(auto) col_EeW23s = out_2LuaMH->get_col<0>();
decltype(auto) col_5gY1Dm = out_2LuaMH->get_col<1>();
typedef record<decays<decltype(mont_8AE)::value_t>> record_typegj3e8Xf;
ankerl::unordered_dense::map<record_typegj3e8Xf, uint32_t, transTypes<record_typegj3e8Xf, hasher>> gMzMTEvd;
gMzMTEvd.reserve(mont_8AE.size);
uint32_t* reversemap = new uint32_t[mont_8AE.size<<1], 
	*mapbase = reversemap + mont_8AE.size;
for (uint32_t i2E = 0; i2E < mont_8AE.size; ++i2E){
	reversemap[i2E] = gMzMTEvd.hashtable_push(forward_as_tuple(mont_8AE[i2E]));
}
auto arr_values = gMzMTEvd.values().data();
auto arr_len = gMzMTEvd.size();
uint32_t* seconds = new uint32_t[gMzMTEvd.size()];

auto vecs = static_cast<vector_type<uint32_t>*>(malloc(sizeof(vector_type<uint32_t>) * arr_len));
vecs[0].init_from(arr_values[0].second, mapbase);
for (uint32_t i = 1; i < arr_len; ++i) {
	vecs[i].init_from(arr_values[i].second, mapbase + arr_values[i - 1].second);
	arr_values[i].second += arr_values[i - 1].second;
}
for (uint32_t i = 0; i < mont_8AE.size; ++i) {
	auto id = reversemap[i];
	mapbase[--arr_values[id].second] = i;    
}

col_EeW23s.reserve(gMzMTEvd.size());
col_5gY1Dm.reserve(gMzMTEvd.size());
auto buf_col_5gY1Dm = new double[mont_8AE.size];
for (uint32_t i = 0; i < arr_len; ++i) {
	col_5gY1Dm[i].init_from(vecs[i].size, buf_col_5gY1Dm + arr_values[i].second);
}
for (uint32_t i = 0; i < arr_len; ++i) {

auto &key_3iNX3qG = arr_values[i].first;
auto &val_7jjv8Mo = arr_values[i].second;
col_EeW23s.emplace_back(get<0>(key_3iNX3qG));

avgw(10, sales_2RB[vecs[i]], col_5gY1Dm[i]);

}
//print(*out_2LuaMH);
//FILE* fp_5LQeym = fopen("flatten.csv", "wb");
out_2LuaMH->printall(",", "\n", nullptr, nullptr, 10);
//fclose(fp_5LQeym);
puts("done.");
return 0;
}