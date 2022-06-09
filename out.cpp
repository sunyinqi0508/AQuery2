#include "./server/aggregations.h"
#include <unordered_map>
#include "csv.h"
#include "./server/hasher.h"
#include "./server/libaquery.h"

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
io::CSVReader<4> csv_reader_6pX2fy("test.csv");
csv_reader_6pX2fy.read_header(io::ignore_extra_column, "a","b","c","d");
int tmp_1E7DxvaO;
int tmp_5JwTTT4O;
int tmp_3gbplDAu;
int tmp_XK4BgA6z;
while(csv_reader_6pX2fy.read_row(tmp_1E7DxvaO,tmp_5JwTTT4O,tmp_3gbplDAu,tmp_XK4BgA6z)) { 

test_a.emplace_back(tmp_1E7DxvaO);
test_b.emplace_back(tmp_5JwTTT4O);
test_c.emplace_back(tmp_3gbplDAu);
test_d.emplace_back(tmp_XK4BgA6z);
}
typedef record<decltype(test_a[0]),decltype(test_b[0]),decltype(test_d[0])> record_type61iBrX3;
unordered_map<record_type61iBrX3, vector_type<uint32_t>, transTypes<record_type61iBrX3, hasher>> g7sUysrP;
for (uint32_t i2I = 0; i2I < test_a.size; ++i2I){
g7sUysrP[forward_as_tuple(test_a[i2I],test_b[i2I],test_d[i2I])].emplace_back(i2I);
}
auto out_1SHu = new TableInfo<decays<decltype(sum(test_c))>,value_type<decays<decltype(test_b)>>,value_type<decays<decltype(test_d)>>>("out_1SHu", 3);
cxt->tables.insert({"out_1SHu", out_1SHu});
auto& out_1SHu_sumtestc = *(ColRef<decays<decltype(sum(test_c))>> *)(&out_1SHu->colrefs[0]);
auto& out_1SHu_b = *(ColRef<value_type<decays<decltype(test_b)>>> *)(&out_1SHu->colrefs[1]);
auto& out_1SHu_d = *(ColRef<value_type<decays<decltype(test_d)>>> *)(&out_1SHu->colrefs[2]);
out_1SHu_sumtestc.init("sumtestc");
out_1SHu_b.init("b");
out_1SHu_d.init("d");
for(auto& i64 : g7sUysrP) {
auto &key_645kbJO = i64.first;
auto &val_1UnUa89 = i64.second;
out_1SHu_sumtestc.emplace_back(sum(test_c[val_1UnUa89]));
out_1SHu_b.emplace_back(get<1>(key_645kbJO));
out_1SHu_d.emplace_back(get<2>(key_645kbJO));
}
auto d6tAcglo = out_1SHu->order_by_view<-3,1>();
print(d6tAcglo);
return 0;
}