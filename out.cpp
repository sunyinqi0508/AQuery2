#include <unordered_map>
#include "./server/libaquery.h"
#include "./server/hasher.h"
#include "./server/aggregations.h"
#include "csv.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    auto test = new TableInfo<int,int,int,int>("test", 4);
cxt->tables.insert({"test", test});
auto& test_a = *(ColRef<int> *)(&test->colrefs[0]);
auto& test_b = *(ColRef<int> *)(&test->colrefs[1]);
auto& test_c = *(ColRef<int> *)(&test->colrefs[2]);
auto& test_d = *(ColRef<int> *)(&test->colrefs[3]);
test_a.init();
test_b.init();
test_c.init();
test_d.init();
io::CSVReader<4> csv_reader_4bTMJ9("test.csv");
csv_reader_4bTMJ9.read_header(io::ignore_extra_column, "a","b","c","d");
int tmp_78E1nhZJ;
int tmp_4wnHGd9t;
int tmp_5OL9GlRp;
int tmp_155GVQC6;
while(csv_reader_4bTMJ9.read_row(tmp_78E1nhZJ,tmp_4wnHGd9t,tmp_5OL9GlRp,tmp_155GVQC6)) { 

test_a.emplace_back(tmp_78E1nhZJ);
test_b.emplace_back(tmp_4wnHGd9t);
test_c.emplace_back(tmp_5OL9GlRp);
test_d.emplace_back(tmp_155GVQC6);
}
typedef record<decltype(test_a[0]),decltype(test_b[0]),decltype(test_d[0])> record_type6jn8Y49;
unordered_map<record_type6jn8Y49, vector_type<uint32_t>, transTypes<record_type6jn8Y49, hasher>> g5gn6KEb;
for (uint32_t i3V = 0; i3V < test_a.size; ++i3V){
g5gn6KEb[forward_as_tuple(test_a[i3V],test_b[i3V],test_d[i3V])].emplace_back(i3V);
}
auto out_4DCN = new TableInfo<decays<decltype(sum(test_c))>,value_type<decays<decltype(test_b)>>,value_type<decays<decltype(test_d)>>>("out_4DCN", 3);
cxt->tables.insert({"out_4DCN", out_4DCN});
auto& out_4DCN_sumtestc = *(ColRef<decays<decltype(sum(test_c))>> *)(&out_4DCN->colrefs[0]);
auto& out_4DCN_b = *(ColRef<value_type<decays<decltype(test_b)>>> *)(&out_4DCN->colrefs[1]);
auto& out_4DCN_d = *(ColRef<value_type<decays<decltype(test_d)>>> *)(&out_4DCN->colrefs[2]);
auto lineage = test->bind(out_4DCN);
out_4DCN_sumtestc.init();
out_4DCN_b.init();
out_4DCN_d.init();
for(auto& i1s : g5gn6KEb) {
auto &key_4Q0aEyH = i1s.first;
auto &val_7BUMR6d = i1s.second;
out_4DCN_sumtestc.emplace_back(sum(test_c[val_7BUMR6d]));
out_4DCN_b.emplace_back(get<1>(key_4Q0aEyH));
out_4DCN_d.emplace_back(get<2>(key_4Q0aEyH));
lineage.emplace_back(val_7BUMR6d[0]);
}
print(lineage.rid);
auto d6X0PMzl = out_4DCN->order_by_view<-3,1>();
print(d6X0PMzl);
return 0;
}