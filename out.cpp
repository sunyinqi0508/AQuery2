#include "csv.h"
#include <unordered_map>
#include "./server/libaquery.h"
#include "./server/hasher.h"
#include "./server/aggregations.h"

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
io::CSVReader<4> csv_reader_6qlGpe("test.csv");
csv_reader_6qlGpe.read_header(io::ignore_extra_column, "a","b","c","d");
int tmp_39gHMkie;
int tmp_190h2sZs;
int tmp_4a8dDzSN;
int tmp_3LAKxSmM;
while(csv_reader_6qlGpe.read_row(tmp_39gHMkie,tmp_190h2sZs,tmp_4a8dDzSN,tmp_3LAKxSmM)) { 

test_a.emplace_back(tmp_39gHMkie);
test_b.emplace_back(tmp_190h2sZs);
test_c.emplace_back(tmp_4a8dDzSN);
test_d.emplace_back(tmp_3LAKxSmM);
}
typedef record<decltype(test_a[0]),decltype(test_b[0]),decltype(test_d[0])> record_type2Te4GFo;
unordered_map<record_type2Te4GFo, vector_type<uint32_t>, transTypes<record_type2Te4GFo, hasher>> g79JNXM8;
for (uint32_t i5x = 0; i5x < test_a.size; ++i5x){
g79JNXM8[forward_as_tuple(test_a[i5x],test_b[i5x],test_d[i5x])].emplace_back(i5x);
}
auto out_5NL7 = new TableInfo<decays<decltype(sum(test_c[0]))>,decays<decltype(test_b[0])>,decays<decltype(test_d[0])>>("out_5NL7", 3);
cxt->tables.insert({"out_5NL7", out_5NL7});
auto& out_5NL7_sumtestc = *(ColRef<decays<decltype(sum(test_c[0]))>> *)(&out_5NL7->colrefs[0]);
auto& out_5NL7_get1None = *(ColRef<decays<decltype(test_b[0])>> *)(&out_5NL7->colrefs[1]);
auto& out_5NL7_get2None = *(ColRef<decays<decltype(test_d[0])>> *)(&out_5NL7->colrefs[2]);
out_5NL7_sumtestc.init();
out_5NL7_get1None.init();
out_5NL7_get2None.init();
for(auto& i4l : g79JNXM8) {
auto &key_ADPihOU = i4l.first;
auto &val_7LsrkDP = i4l.second;
out_5NL7_sumtestc.emplace_back(sum(test_c[val_7LsrkDP]));
out_5NL7_get1None.emplace_back(get<1>(key_ADPihOU));
out_5NL7_get2None.emplace_back(get<2>(key_ADPihOU));
}
print(*out_5NL7);

return 0;
}