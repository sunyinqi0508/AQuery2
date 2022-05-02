#include <unordered_map>
#include "./server/aggregations.h"
#include "csv.h"
#include "./server/libaquery.h"
#include "./server/hasher.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    auto sale = new TableInfo<int,int>("sale", 2);
cxt->tables.insert({"sale", sale});
auto& sale_Month = *(ColRef<int> *)(&sale->colrefs[0]);
auto& sale_sales = *(ColRef<int> *)(&sale->colrefs[1]);
sale_Month.init("Month");
sale_sales.init("sales");
io::CSVReader<2> csv_reader_6ojNrU("moving_avg.csv");
csv_reader_6ojNrU.read_header(io::ignore_extra_column, "Month","sales");
int tmp_30abZdE5;
int tmp_zx6KcpzH;
while(csv_reader_6ojNrU.read_row(tmp_30abZdE5,tmp_zx6KcpzH)) { 

sale_Month.emplace_back(tmp_30abZdE5);
sale_sales.emplace_back(tmp_zx6KcpzH);
}
auto out_4oKV = new TableInfo<value_type<decays<decltype(sale_Month)>>,value_type<decays<decltype(avgw(3,sale_sales))>>>("out_4oKV", 2);
cxt->tables.insert({"out_4oKV", out_4oKV});
auto& out_4oKV_Month = *(ColRef<value_type<decays<decltype(sale_Month)>>> *)(&out_4oKV->colrefs[0]);
auto& out_4oKV_avgw3salesales = *(ColRef<value_type<decays<decltype(avgw(3,sale_sales))>>> *)(&out_4oKV->colrefs[1]);
auto order_3t9jQY = sale->order_by<0>();
out_4oKV_Month.init("Month");
out_4oKV_Month = sale_Month[*order_3t9jQY];
out_4oKV_avgw3salesales.init("avgw3salesales");
out_4oKV_avgw3salesales = avgw(3,sale_sales[*order_3t9jQY]);
print(*out_4oKV);
FILE* fp_d7p2ph = fopen("moving_avg_output.csv", "w");
out_4oKV->printall(";", "\n", nullptr, fp_d7p2ph);
fclose(fp_d7p2ph);
typedef record<decltype(sale_sales[0])> record_typexsfbsFs;
unordered_map<record_typexsfbsFs, vector_type<uint32_t>, transTypes<record_typexsfbsFs, hasher>> g5N8IBNq;
for (uint32_t i4w = 0; i4w < sale_sales.size; ++i4w){
g5N8IBNq[forward_as_tuple(sale_sales[i4w])].emplace_back(i4w);
}
auto out_7JGJ = new TableInfo<decays<decltype(sale_Month)>,value_type<decays<decltype(minw(2,sale_sales))>>>("out_7JGJ", 2);
cxt->tables.insert({"out_7JGJ", out_7JGJ});
auto& out_7JGJ_Month = *(ColRef<decays<decltype(sale_Month)>> *)(&out_7JGJ->colrefs[0]);
auto& out_7JGJ_minw2salesales = *(ColRef<value_type<decays<decltype(minw(2,sale_sales))>>> *)(&out_7JGJ->colrefs[1]);
out_7JGJ_Month.init("Month");
out_7JGJ_minw2salesales.init("minw2salesales");
for(auto& iVb : g5N8IBNq) {
auto &val_6xjJXey = iVb.second;
sale->order_by<-1>(&val_6xjJXey);
}
for(auto& i5G : g5N8IBNq) {
auto &key_1e9JJOf = i5G.first;
auto &val_6g6wlkk = i5G.second;
out_7JGJ_Month.emplace_back(sale_Month[val_6g6wlkk]);
out_7JGJ_minw2salesales.emplace_back(minw(2,get<0>(key_1e9JJOf)));
}
print(*out_7JGJ);
FILE* fp_1yhzJM = fopen("flatten.csv", "w");
out_7JGJ->printall(",", "\n", nullptr, fp_1yhzJM);
fclose(fp_1yhzJM);
return 0;
}