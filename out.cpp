#include "./server/libaquery.h"
#include "./server/aggregations.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    auto stocks = new TableInfo<int,int>("stocks", 2);
cxt->tables.insert({"stocks", stocks});
auto& stocks_timestamp = *(ColRef<int> *)(&stocks->colrefs[0]);
auto& stocks_price = *(ColRef<int> *)(&stocks->colrefs[1]);
stocks_timestamp.init();
stocks_price.init();
stocks_timestamp.emplace_back(1);
stocks_price.emplace_back(15);
stocks_timestamp.emplace_back(2);
stocks_price.emplace_back(19);
stocks_timestamp.emplace_back(3);
stocks_price.emplace_back(16);
stocks_timestamp.emplace_back(4);
stocks_price.emplace_back(17);
stocks_timestamp.emplace_back(5);
stocks_price.emplace_back(15);
stocks_timestamp.emplace_back(6);
stocks_price.emplace_back(13);
stocks_timestamp.emplace_back(7);
stocks_price.emplace_back(5);
stocks_timestamp.emplace_back(8);
stocks_price.emplace_back(8);
stocks_timestamp.emplace_back(9);
stocks_price.emplace_back(7);
stocks_timestamp.emplace_back(10);
stocks_price.emplace_back(13);
stocks_timestamp.emplace_back(11);
stocks_price.emplace_back(11);
stocks_timestamp.emplace_back(12);
stocks_price.emplace_back(14);
stocks_timestamp.emplace_back(13);
stocks_price.emplace_back(10);
stocks_timestamp.emplace_back(14);
stocks_price.emplace_back(5);
stocks_timestamp.emplace_back(15);
stocks_price.emplace_back(2);
stocks_timestamp.emplace_back(16);
stocks_price.emplace_back(5);
auto out_oMxe = new TableInfo<value_type<decays<decltype(max((stocks_price-min(stocks_timestamp))))>>>("out_oMxe", 1);
cxt->tables.insert({"out_oMxe", out_oMxe});
auto& out_oMxe_maxstockspriceminstockstimestamp = *(ColRef<value_type<decays<decltype(max((stocks_price-min(stocks_timestamp))))>>> *)(&out_oMxe->colrefs[0]);
out_oMxe_maxstockspriceminstockstimestamp.init();
out_oMxe_maxstockspriceminstockstimestamp = max((stocks_price-min(stocks_timestamp)));
print(*out_oMxe);
auto out_2ZWg = new TableInfo<value_type<decays<decltype(max((stocks_price-mins(stocks_price))))>>>("out_2ZWg", 1);
cxt->tables.insert({"out_2ZWg", out_2ZWg});
auto& out_2ZWg_maxstockspriceminsstocksprice = *(ColRef<value_type<decays<decltype(max((stocks_price-mins(stocks_price))))>>> *)(&out_2ZWg->colrefs[0]);
out_2ZWg_maxstockspriceminsstocksprice.init();
out_2ZWg_maxstockspriceminsstocksprice = max((stocks_price-mins(stocks_price)));
print(*out_2ZWg);
const auto& tmp_sz_B19sAY = stocks_timestamp.size;
auto out_JZsz = new TableInfo<value_type<decays<decltype(stocks_price)>>,value_type<decays<decltype(stocks_timestamp)>>>("out_JZsz", 2);
cxt->tables.insert({"out_JZsz", out_JZsz});
auto& out_JZsz_price = *(ColRef<value_type<decays<decltype(stocks_price)>>> *)(&out_JZsz->colrefs[0]);
auto& out_JZsz_timestamp = *(ColRef<value_type<decays<decltype(stocks_timestamp)>>> *)(&out_JZsz->colrefs[1]);
out_JZsz_price.init();
out_JZsz_timestamp.init();
for (uint32_t i3D = 0; i3D < tmp_sz_B19sAY; ++i3D){
if ((((stocks_price[i3D]-stocks_timestamp[i3D])>=1)&&!(((stocks_price[i3D]*stocks_timestamp[i3D])<100)))) {
out_JZsz_price = stocks_price[i3D];
out_JZsz_timestamp = stocks_timestamp[i3D];
}}
print(*out_JZsz);
auto out_2Fm0 = new TableInfo<value_type<decays<decltype(max((stocks_price-mins(stocks_price))))>>>("out_2Fm0", 1);
cxt->tables.insert({"out_2Fm0", out_2Fm0});
auto& out_2Fm0_maxstockspriceminsstocksprice = *(ColRef<value_type<decays<decltype(max((stocks_price-mins(stocks_price))))>>> *)(&out_2Fm0->colrefs[0]);
auto order_5qqTLa = stocks->order_by<-1>();
out_2Fm0_maxstockspriceminsstocksprice.init();
out_2Fm0_maxstockspriceminsstocksprice = max((stocks_price[*order_5qqTLa]-mins(stocks_price[*order_5qqTLa])));
print(*out_2Fm0);
return 0;
}