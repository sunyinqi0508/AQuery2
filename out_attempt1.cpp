#include "./server/libaquery.h"
#include <unordered_map>
#include "./server/hasher.h"
#include "csv.h"
#include "./server/aggregations.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    auto sale = new TableInfo<int,int>("sale", 2);
cxt->tables.insert({"sale", sale});
auto& sale_Month = *(ColRef<int> *)(&sale->colrefs[0]);
auto& sale_sales = *(ColRef<int> *)(&sale->colrefs[1]);
sale_Month.init();
sale_sales.init();
io::CSVReader<2> csv_reader_53ychC("moving_avg.csv");
csv_reader_53ychC.read_header(io::ignore_extra_column, "Month","sales");
int tmp_7ttMnHd3;
int tmp_5nHjeAtP;
while(csv_reader_53ychC.read_row(tmp_7ttMnHd3,tmp_5nHjeAtP)) { 

sale_Month.emplace_back(tmp_7ttMnHd3);
sale_sales.emplace_back(tmp_5nHjeAtP);
}
auto out_3Xio = new TableInfo<decays<decltype(sale_Month[0])>,decays<decltype(avgw(3,sale_sales))>>("out_3Xio", 2);
cxt->tables.insert({"out_3Xio", out_3Xio});
auto& out_3Xio_Month = *(ColRef<decays<decltype(sale_Month[0])>> *)(&out_3Xio->colrefs[0]);
auto& out_3Xio_avgsw3salesales = *(ColRef<decays<decltype(avgw(3,sale_sales))>> *)(&out_3Xio->colrefs[1]);
out_3Xio_Month.init();
out_3Xio_Month = sale_Month;
out_3Xio_avgsw3salesales.init();
out_3Xio_avgsw3salesales = avgw(3,sale_sales);
// print(*out_3Xio);
FILE* fp_4nKGhD = fopen("moving_avg_output.csv", "w");
out_3Xio->printall(",", "\n", nullptr, fp_4nKGhD);
fclose(fp_4nKGhD);
typedef record<decltype(sale_sales[0])> record_type1H2vDGL;
unordered_map<record_type1H2vDGL, vector_type<uint32_t>, transTypes<record_type1H2vDGL, hasher>> g6Mjxfk5;
for (uint32_t i7u = 0; i7u < sale_sales.size; ++i7u){
g6Mjxfk5[forward_as_tuple(sale_sales[i7u])].emplace_back(i7u);
}
auto out_2IU2 = new TableInfo<decays<decltype(sale_sales[0])>,decays<decltype(minw(2,sale_Month))>>("out_2IU2", 2);
cxt->tables.insert({"out_2IU2", out_2IU2});
auto& out_2IU2_sales = *(ColRef<decays<decltype(sale_sales[0])>> *)(&out_2IU2->colrefs[0]);
auto& out_2IU2_minsw2saleMonth = *(ColRef<decays<decltype(minw(2,sale_Month))>> *)(&out_2IU2->colrefs[1]);
out_2IU2_sales.init();
out_2IU2_minsw2saleMonth.init();
for(auto& i5J : g6Mjxfk5) {
auto &key_4jl5toH = i5J.first;
auto &val_VJGwVwH = i5J.second;
out_2IU2_sales.emplace_back(get<0>(key_4jl5toH));
out_2IU2_minsw2saleMonth.emplace_back(minw(2,sale_Month[val_VJGwVwH]));
}
// print(*out_2IU2);
FILE* fp_18R4fY = fopen("flatten.csv", "w");
out_2IU2->printall(",","\n", nullptr, fp_18R4fY);
fclose(fp_18R4fY);
return 0;
}