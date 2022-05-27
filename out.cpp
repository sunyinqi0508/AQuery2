#include "./server/aggregations.h"
#include "csv.h"
#include "./server/libaquery.h"
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
io::CSVReader<4> csv_reader_1qh80y("test.csv");
csv_reader_1qh80y.read_header(io::ignore_extra_column, "a","b","c","d");
int tmp_6JfuovoZ;
int tmp_4B4ADRgW;
int tmp_3JHd3elW;
int tmp_1heR8kZw;
while(csv_reader_1qh80y.read_row(tmp_6JfuovoZ,tmp_4B4ADRgW,tmp_3JHd3elW,tmp_1heR8kZw)) { 

test_a.emplace_back(tmp_6JfuovoZ);
test_b.emplace_back(tmp_4B4ADRgW);
test_c.emplace_back(tmp_3JHd3elW);
test_d.emplace_back(tmp_1heR8kZw);
}
typedef record<decltype(test_a[0]),decltype(test_b[0]),decltype(test_d[0])> record_type3he5qd1;
unordered_map<record_type3he5qd1, vector_type<uint32_t>, transTypes<record_type3he5qd1, hasher>> g59vWI2v;
for (uint32_t i3P = 0; i3P < test_a.size; ++i3P){
g59vWI2v[forward_as_tuple(test_a[i3P],test_b[i3P],test_d[i3P])].emplace_back(i3P);
}
auto out_6JAp = new TableInfo<decays<decltype(sum(test_c))>,value_type<decays<decltype(test_b)>>,value_type<decays<decltype(test_d)>>>("out_6JAp", 3);
cxt->tables.insert({"out_6JAp", out_6JAp});
auto& out_6JAp_sumtestc = *(ColRef<decays<decltype(sum(test_c))>> *)(&out_6JAp->colrefs[0]);
auto& out_6JAp_b = *(ColRef<value_type<decays<decltype(test_b)>>> *)(&out_6JAp->colrefs[1]);
auto& out_6JAp_d = *(ColRef<value_type<decays<decltype(test_d)>>> *)(&out_6JAp->colrefs[2]);
out_6JAp_sumtestc.init("sumtestc");
out_6JAp_b.init("b");
out_6JAp_d.init("d");
for(auto& i2Y : g59vWI2v) {
auto &key_1yBYhdd = i2Y.first;
auto &val_61QXy6G = i2Y.second;
out_6JAp_sumtestc.emplace_back(sum(test_c[val_61QXy6G]));
out_6JAp_b.emplace_back(get<1>(key_1yBYhdd));
out_6JAp_d.emplace_back(get<2>(key_1yBYhdd));
}
auto d1dyPtv0 = out_6JAp->order_by_view<-3,1>();
print(d1dyPtv0);
return 0;
}