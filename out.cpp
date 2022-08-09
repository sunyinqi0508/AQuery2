#include "./server/libaquery.h"
#include <unordered_map>
#include "./server/monetdb_conn.h"
#include "./server/hasher.h"
#include "./server/aggregations.h"
__AQEXPORT__(int) dll_3V1c8v(Context* cxt) {
	using namespace std;
	using namespace types;
	auto server = static_cast<Server*>(cxt->alt_server);
auto len_5Bixfq = server->cnt;
auto mont_1Mz = ColRef<int>(len_5Bixfq, server->getCol(0));
auto sales_2DN = ColRef<int>(len_5Bixfq, server->getCol(1));
auto out_5Ffhnc = new TableInfo<int,double>("out_5Ffhnc");
out_5Ffhnc->get_col<0>().initfrom(mont_1Mz);
out_5Ffhnc->get_col<1>() = avgw(3, sales_2DN);
print(*out_5Ffhnc);
FILE* fp_57Mh6f = fopen("moving_avg_output.csv", "w");
out_5Ffhnc->printall(";", "\n", nullptr, fp_57Mh6f);
fclose(fp_57Mh6f);
puts("done.");
return 0;
}
__AQEXPORT__(int) dll_frRsQ7(Context* cxt) {
	using namespace std;
	using namespace types;
	auto server = static_cast<Server*>(cxt->alt_server);
auto len_5JydG3 = server->cnt;
auto mont_34m = ColRef<int>(len_5JydG3, server->getCol(0));
auto sales_4wo = ColRef<int>(len_5JydG3, server->getCol(1));
auto out_4Fsh6O = new TableInfo<int,ColRef<int>>("out_4Fsh6O");
decltype(auto) col_4A04Hh = out_4Fsh6O->get_col<0>();
decltype(auto) col_1S5CS2 = out_4Fsh6O->get_col<1>();
auto t4HUjxwQ = mont_34m;
typedef record<decays<decltype(t4HUjxwQ)::value_t>> record_type5PGugsV;
unordered_map<record_type5PGugsV, vector_type<uint32_t>, transTypes<record_type5PGugsV, hasher>> g6HIDqpq;
for (uint32_t ilq = 0; ilq < t4HUjxwQ.size; ++ilq){
g6HIDqpq[forward_as_tuple(t4HUjxwQ[ilq])].emplace_back(ilq);
}
for (auto& i2M : g6HIDqpq) {
auto &key_5JcLJMV = i2M.first;
auto &val_yqDe0lt = i2M.second;
col_4A04Hh.emplace_back(get<0>(key_5JcLJMV));

col_1S5CS2.emplace_back(minw(2, sales_4wo[val_yqDe0lt]));

}
print(*out_4Fsh6O);
FILE* fp_6UC6Yg = fopen("flatten.csv", "w");
out_4Fsh6O->printall(",", "\n", nullptr, fp_6UC6Yg);
fclose(fp_6UC6Yg);
puts("done.");
return 0;
}