#include "./server/hasher.h"
#include "./server/aggregations.h"
#include "csv.h"
#include "./server/libaquery.h"
#include <unordered_map>

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    auto sale = new TableInfo<int,int>("sale", 2);
cxt->tables.insert({"sale", sale});
auto& sale_Month = *(ColRef<int> *)(&sale->colrefs[0]);
auto& sale_sales = *(ColRef<int> *)(&sale->colrefs[1]);
sale_Month.init();
sale_sales.init();
io::CSVReader<2> csv_reader_6T89Ll("moving_avg.csv");
csv_reader_6T89Ll.read_header(io::ignore_extra_column, "Month","sales");
int tmp_5vttJ2yV;
int tmp_2ckq15YU;
while(csv_reader_6T89Ll.read_row(tmp_5vttJ2yV,tmp_2ckq15YU)) { 

sale_Month.emplace_back(tmp_5vttJ2yV);
sale_sales.emplace_back(tmp_2ckq15YU);
}
auto out_2UiD = new TableInfo<value_type<decays<decltype(sale_Month)>>,value_type<decays<decltype(avgw(3,sale_sales))>>>("out_2UiD", 2);
cxt->tables.insert({"out_2UiD", out_2UiD});
auto& out_2UiD_Month = *(ColRef<value_type<decays<decltype(sale_Month)>>> *)(&out_2UiD->colrefs[0]);
auto& out_2UiD_avgw3salesales = *(ColRef<value_type<decays<decltype(avgw(3,sale_sales))>>> *)(&out_2UiD->colrefs[1]);
auto order_1NNZ9F = sale->order_by<0>();
out_2UiD_Month.init();
out_2UiD_Month = sale_Month[*order_1NNZ9F];
out_2UiD_avgw3salesales.init();
out_2UiD_avgw3salesales = avgw(3,sale_sales[*order_1NNZ9F]);
print(*out_2UiD);
FILE* fp_6xIJn4 = fopen("moving_avg_output.csv", "w");
out_2UiD->printall(";", "\n", nullptr, fp_6xIJn4);
fclose(fp_6xIJn4);
typedef record<decltype(sale_sales[0])> record_type6Lepq5T;
unordered_map<record_type6Lepq5T, vector_type<uint32_t>, transTypes<record_type6Lepq5T, hasher>> g4loWjmn;
for (uint32_t i5g = 0; i5g < sale_sales.size; ++i5g){
g4loWjmn[forward_as_tuple(sale_sales[i5g])].emplace_back(i5g);
}
auto out_2YlO = new TableInfo<value_type<decays<decltype(sale_sales)>>,decays<decltype(minw(2,sale_Month))>>("out_2YlO", 2);
cxt->tables.insert({"out_2YlO", out_2YlO});
auto& out_2YlO_sales = *(ColRef<value_type<decays<decltype(sale_sales)>>> *)(&out_2YlO->colrefs[0]);
auto& out_2YlO_minw2saleMonth = *(ColRef<decays<decltype(minw(2,sale_Month))>> *)(&out_2YlO->colrefs[1]);
out_2YlO_sales.init();
out_2YlO_minw2saleMonth.init();
for(auto& iFU : g4loWjmn) {
auto &key_3AwvKMR = iFU.first;
auto &val_7jtE12E = iFU.second;
out_2YlO_sales.emplace_back(get<0>(key_3AwvKMR));
out_2YlO_minw2saleMonth.emplace_back(minw(2,sale_Month[val_7jtE12E]));
}
print(*out_2YlO);
FILE* fp_45ld6S = fopen("flatten.csv", "w");
out_2YlO->printall(",", "\n", nullptr, fp_45ld6S);
fclose(fp_45ld6S);
return 0;
}