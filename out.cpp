#include "./server/libaquery.h"
#include "./server/aggregations.h"
#include "csv.h"
#include "./server/hasher.h"
#include <unordered_map>

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    auto test = new TableInfo<int,int,int,int>("test", 4);
cxt->tables.insert({"test", test});
auto& test_a = *(ColRef<int> *)(&test->colrefs[0]);
auto& test_b = *(ColRef<int> *)(&test->colrefs[1]);
auto& test_c = *(ColRef<int> *)(&test->colrefs[2]);
auto& test_d = *(ColRef<int> *)(&test->colrefs[3]);
test_a.init("a");
test_b.init("b");
test_c.init("c");
test_d.init("d");
io::CSVReader<4> csv_reader_307VD4("test.csv");
csv_reader_307VD4.read_header(io::ignore_extra_column, "a","b","c","d");
int tmp_3LXIYQmp;
int tmp_1m5NCKR4;
int tmp_10LZcLgy;
int tmp_39pPZL8W;
while(csv_reader_307VD4.read_row(tmp_3LXIYQmp,tmp_1m5NCKR4,tmp_10LZcLgy,tmp_39pPZL8W)) { 

test_a.emplace_back(tmp_3LXIYQmp);
test_b.emplace_back(tmp_1m5NCKR4);
test_c.emplace_back(tmp_10LZcLgy);
test_d.emplace_back(tmp_39pPZL8W);
}
typedef record<decltype(test_a[0]),decltype(test_b[0]),decltype(test_d[0])> record_type3OMslKw;
unordered_map<record_type3OMslKw, vector_type<uint32_t>, transTypes<record_type3OMslKw, hasher>> g7LNVAss;
for (uint32_t i1T = 0; i1T < test_a.size; ++i1T){
g7LNVAss[forward_as_tuple(test_a[i1T],test_b[i1T],test_d[i1T])].emplace_back(i1T);
}
auto out_HSfK = new TableInfo<decays<decltype(sum(test_c))>,value_type<decays<decltype(test_b)>>,value_type<decays<decltype(test_d)>>>("out_HSfK", 3);
cxt->tables.insert({"out_HSfK", out_HSfK});
auto& out_HSfK_sumtestc = *(ColRef<decays<decltype(sum(test_c))>> *)(&out_HSfK->colrefs[0]);
auto& out_HSfK_b = *(ColRef<value_type<decays<decltype(test_b)>>> *)(&out_HSfK->colrefs[1]);
auto& out_HSfK_d = *(ColRef<value_type<decays<decltype(test_d)>>> *)(&out_HSfK->colrefs[2]);
out_HSfK_sumtestc.init("sumtestc");
out_HSfK_b.init("b");
out_HSfK_d.init("d");
for(auto& i18 : g7LNVAss) {
auto &key_3s5slnK = i18.first;
auto &val_2nNLv0D = i18.second;
out_HSfK_sumtestc.emplace_back(sum(test_c[val_2nNLv0D]));
out_HSfK_b.emplace_back(get<1>(key_3s5slnK));
out_HSfK_d.emplace_back(get<2>(key_3s5slnK));
}
auto d5b7C95U = out_HSfK->order_by_view<-3,1>();
print(d5b7C95U);
return 0;
}