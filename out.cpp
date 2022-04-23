#include "csv.h"
#include "./server/hasher.h"
#include <unordered_map>
#include "./server/aggregations.h"
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
test_a.init();
test_b.init();
test_c.init();
test_d.init();
io::CSVReader<4> csv_reader_53LkPG("test.csv");
csv_reader_53LkPG.read_header(io::ignore_extra_column, "a","b","c","d");
int tmp_43xeYChp;
int tmp_3Vnt4fLK;
int tmp_1HKZwQBO;
int tmp_6IwJuIpg;
while(csv_reader_53LkPG.read_row(tmp_43xeYChp,tmp_3Vnt4fLK,tmp_1HKZwQBO,tmp_6IwJuIpg)) { 

test_a.emplace_back(tmp_43xeYChp);
test_b.emplace_back(tmp_3Vnt4fLK);
test_c.emplace_back(tmp_1HKZwQBO);
test_d.emplace_back(tmp_6IwJuIpg);
}
typedef record<decltype(test_a[0]),decltype(test_b[0]),decltype(test_d[0])> record_type1CmZCvh;
unordered_map<record_type1CmZCvh, vector_type<uint32_t>, transTypes<record_type1CmZCvh, hasher>> g6nov6MR;
for (uint32_t i4I = 0; i4I < test_a.size; ++i4I){
g6nov6MR[forward_as_tuple(test_a[i4I],test_b[i4I],test_d[i4I])].emplace_back(i4I);
}
auto out_684r = new TableInfo<decays<decltype(sum(test_c[0]))>,decays<decltype(test_b[0])>,decays<decltype(test_d[0])>>("out_684r", 3);
cxt->tables.insert({"out_684r", out_684r});
auto& out_684r_sumtestc = *(ColRef<decays<decltype(sum(test_c[0]))>> *)(&out_684r->colrefs[0]);
auto& out_684r_b = *(ColRef<decays<decltype(test_b[0])>> *)(&out_684r->colrefs[1]);
auto& out_684r_d = *(ColRef<decays<decltype(test_d[0])>> *)(&out_684r->colrefs[2]);
out_684r_sumtestc.init();
out_684r_b.init();
out_684r_d.init();
for(auto& i3d : g6nov6MR) {
auto &key_1TaM8D7 = i3d.first;
auto &val_129np3x = i3d.second;
out_684r_sumtestc.emplace_back(sum(test_c[val_129np3x]));
out_684r_b.emplace_back(get<1>(key_1TaM8D7));
out_684r_d.emplace_back(get<2>(key_1TaM8D7));
}
auto d2X3bP6l =out_684r->order_by_view<-3,1>();
puts("a");
print(*(out_684r->order_by<-3,1>()));
puts("b");
print(out_684r->order_by_view<-3,1>());
puts("e");
print(*out_684r);
return 0;
}